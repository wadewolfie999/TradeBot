#include "PortfolioAllocator.hpp"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <numeric>

PortfolioAllocator::PortfolioAllocator(double longThreshold,
                                       double shortThreshold)
    : m_longThreshold(longThreshold)
    , m_shortThreshold(shortThreshold)
{}

// ── Phase 10 static weight API ────────────────────────────────────────────────

void PortfolioAllocator::setWeight(const std::string& strategyId, double weight)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_weights[strategyId] = weight;
}

double PortfolioAllocator::getWeight(const std::string& strategyId) const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_weights.find(strategyId);
    return (it != m_weights.end()) ? it->second : 0.0;
}

// ── Phase 11 telemetry ────────────────────────────────────────────────────────

void PortfolioAllocator::recomputePhi(PhiState& ps) const noexcept
{
    if (ps.pnlWindow.empty()) { ps.phi = 0.0; return; }

    double grossProfit = 0.0;
    double grossLoss   = 0.0;
    for (double pnl : ps.pnlWindow) {
        if (pnl >= 0.0) { grossProfit += pnl; }
        else            { grossLoss   += std::abs(pnl); }
    }

    double pf;
    if (grossLoss == 0.0) {
        pf = (grossProfit > 0.0) ? 10.0 : 1.0;
    } else {
        pf = grossProfit / grossLoss;
    }

    // Map profit factor to [-1, 1]:
    //   PF = 1.0 -> Phi = 0.0 (break-even)
    //   PF > 1.0 -> positive Phi, capped at +1.0 at PF = 3.0
    //   PF < 1.0 -> negative Phi, capped at -1.0 at PF = 0.0
    //   Formula: Phi = (PF - 1.0) / 2.0, clamped to [-1, 1]
    ps.phi = std::max(-1.0, std::min(1.0, (pf - 1.0) / 2.0));
}

void PortfolioAllocator::onTradeClosed(const std::string& strategyId,
                                       double realizedPnL)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    PhiState& ps = m_phiStates[strategyId];
    if (ps.strategyId.empty()) { ps.strategyId = strategyId; }

    ps.pnlWindow.push_back(realizedPnL);
    if (static_cast<int>(ps.pnlWindow.size()) > PHI_WINDOW) {
        ps.pnlWindow.pop_front();
    }
    recomputePhi(ps);
}

double PortfolioAllocator::getPhi(const std::string& strategyId) const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_phiStates.find(strategyId);
    return (it != m_phiStates.end()) ? it->second.phi : 0.0;
}

// ── Dynamic weight computation ────────────────────────────────────────────────

double PortfolioAllocator::computeDynamicWeight(const std::string& strategyId,
                                                MarketRegime regime) const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto cacheIt = m_cachedDynamicWeights.find(strategyId);
    if (cacheIt != m_cachedDynamicWeights.end()) {
        return cacheIt->second;
    }
    const double m   = RegimeDetector::affinity(strategyId, regime);
    const auto phiIt = m_phiStates.find(strategyId);
    const double phi = (phiIt != m_phiStates.end()) ? phiIt->second.phi : 0.0;
    const double w   = LAMBDA_REGIME * m + LAMBDA_PERF * phi;
    return w;
}

void PortfolioAllocator::recomputeWeights(MarketRegime regime)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cachedDynamicWeights.clear();

    // Compute for all known strategy IDs from static weights and Phi states.
    std::unordered_map<std::string, bool> seen;
    for (const auto& [sid, _] : m_weights) {
        seen[sid] = true;
    }
    for (const auto& [sid, _] : m_phiStates) {
        seen[sid] = true;
    }
    if (seen.empty()) {
        seen["SMA_01"] = true;
        seen["MR_01"]  = true;
    }

    double sumAbs = 0.0;
    for (const auto& [sid, _] : seen) {
        const double m = RegimeDetector::affinity(sid, regime);
        const auto phiIt = m_phiStates.find(sid);
        const double phi = (phiIt != m_phiStates.end()) ? phiIt->second.phi : 0.0;
        const double w = LAMBDA_REGIME * m + LAMBDA_PERF * phi;
        m_cachedDynamicWeights[sid] = w;
        sumAbs += std::abs(w);
    }
    if (sumAbs > 1.0) {
        for (auto& [sid, w] : m_cachedDynamicWeights) {
            w /= sumAbs;
        }
    }
    m_recomputeCount.fetch_add(1);
}

uint64_t PortfolioAllocator::recomputeCount() const noexcept
{
    return m_recomputeCount.load();
}

// ── Ensemble ──────────────────────────────────────────────────────────────────

AllocationResult PortfolioAllocator::ensemble(
    const std::vector<AlphaSignal>& signals) const
{
    AllocationResult result;
    if (signals.empty()) { return result; }

    result.symbol = signals.front().symbol;

    // ── Determine regime and weights ──────────────────────────────────────
    const bool dynamic = (m_regimeDetector != nullptr);
    const MarketRegime regime = dynamic
                                ? m_regimeDetector->currentRegime()
                                : MarketRegime::CHAOS;
    result.regime = regime;

    // Compute per-signal weights
    std::vector<double> weights;
    weights.reserve(signals.size());
    double sumAbsW = 0.0;

    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& sig : signals) {
        double w;
        if (dynamic) {
            auto cacheIt = m_cachedDynamicWeights.find(sig.strategy_id);
            if (cacheIt != m_cachedDynamicWeights.end()) {
                w = cacheIt->second;
            } else {
                const auto phiIt = m_phiStates.find(sig.strategy_id);
                const double phi = (phiIt != m_phiStates.end()) ? phiIt->second.phi : 0.0;
                w = LAMBDA_REGIME * RegimeDetector::affinity(sig.strategy_id, regime)
                  + LAMBDA_PERF * phi;
            }
        } else {
            auto wIt = m_weights.find(sig.strategy_id);
            w = (wIt != m_weights.end()) ? wIt->second : 1.0;
        }
        weights.push_back(w);
        sumAbsW += std::abs(w);
    }

    // Normalise so sum(|W_i|) <= 1.0 (dynamic mode only)
    if (dynamic && sumAbsW > 1.0) {
        for (double& w : weights) { w /= sumAbsW; }
    }

    // ── Weighted-sum conviction ───────────────────────────────────────────
    double totalConviction = 0.0;
    double bestWeightedAbs = -1.0;

    for (std::size_t i = 0; i < signals.size(); ++i) {
        totalConviction += weights[i] * signals[i].conviction;
        const double wAbs = std::abs(weights[i] * signals[i].conviction);
        if (wAbs > bestWeightedAbs) {
            bestWeightedAbs           = wAbs;
            result.dominantStrategyId = signals[i].strategy_id;
        }
    }

    result.totalConviction = totalConviction;

    // ── Threshold gate ────────────────────────────────────────────────────
    if (totalConviction >= m_longThreshold) {
        result.action = Signal::BUY;
    } else if (totalConviction <= -m_shortThreshold) {
        result.action = Signal::SELL;
    } else {
        result.action = Signal::NONE;
    }

    // ── Diagnostic log (regime transitions) ──────────────────────────────
    if (dynamic && result.action != Signal::NONE) {
        std::cout << "[ALLOCATOR] " << result.symbol
                  << " regime=" << RegimeDetector::regimeName(regime)
                  << " S_total=" << std::fixed << std::setprecision(4) << totalConviction
                  << " => " << (result.action == Signal::BUY ? "BUY" : "SELL")
                  << " (dominant=" << result.dominantStrategyId << ")\n";
    } else if (!dynamic && result.action != Signal::NONE) {
        std::cout << "[ALLOCATOR] " << result.symbol
                  << " S_total=" << std::fixed << std::setprecision(4) << totalConviction
                  << " => " << (result.action == Signal::BUY ? "BUY" : "SELL")
                  << " (dominant=" << result.dominantStrategyId << ")\n";
    }

    return result;
}
