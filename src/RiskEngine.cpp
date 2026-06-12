#include "RiskEngine.hpp"
#include "PortfolioManager.hpp"
#include "SystemConfig.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <numeric>
#include <iterator>

RiskEngine::RiskEngine(const PortfolioManager& portfolio,
                       std::size_t maxConcurrentPositions,
                       double      varLimitFraction,
                       std::size_t varWindow) noexcept
    : m_portfolio(portfolio)
    , m_maxConcurrentPositions(maxConcurrentPositions)
    , m_varLimitFraction(varLimitFraction)
    , m_varWindow(varWindow)
{}

bool RiskEngine::canTrade() const noexcept
{
    // Phase 12: circuit breakers take highest priority.
    if (m_halted.load())    { return false; }
    if (m_closeOnly.load()) { return false; }

    if (m_totalDrawdown >= MAX_TOTAL_DRAWDOWN) { return false; }
    if (m_dailyDrawdown >= MAX_DAILY_DRAWDOWN)  { return false; }

    // Use adaptive effective position cap (may be scaled down during high vol).
    const std::size_t positionCap = (m_effectiveMaxPositions > 0)
                                    ? m_effectiveMaxPositions
                                    : m_maxConcurrentPositions;
    if (positionCap > 0 &&
        m_portfolio.getOpenPositionCount() >= positionCap) {
        return false;
    }
    // VaR gate: use adaptive effective VaR limit fraction.
    const double varLimit = (m_effectiveVarLimit > 0.0) ? m_effectiveVarLimit
                                                        : m_varLimitFraction;
    if (varLimit > 0.0 && m_currentVaR95 > 0.0) {
        const double equity = m_portfolio.getTotalEquity();
        if (equity > 0.0 && m_currentVaR95 > equity * varLimit) {
            return false;
        }
    }
    return true;
}

void RiskEngine::setTotalDrawdown(double drawdown) noexcept
{
    m_totalDrawdown = drawdown;
}

void RiskEngine::setDailyDrawdown(double drawdown) noexcept
{
    m_dailyDrawdown = drawdown;
}

void RiskEngine::pushEquityReturn(double currentEquity) noexcept
{
    if (m_prevEquityValid && m_prevEquity > 0.0) {
        const double ret = (currentEquity - m_prevEquity) / m_prevEquity;
        m_returnWindow.push_back(ret);
        if (m_returnWindow.size() > m_varWindow) {
            m_returnWindow.pop_front();
        }

        // Recompute rolling stddev and VaR_95 once we have at least 2 samples.
        if (m_returnWindow.size() >= 2) {
            const double n    = static_cast<double>(m_returnWindow.size());
            const double mean = std::accumulate(m_returnWindow.begin(),
                                                m_returnWindow.end(), 0.0) / n;
            double variance = 0.0;
            for (const double r : m_returnWindow) {
                variance += (r - mean) * (r - mean);
            }
            variance /= n;
            m_returnStdDev  = std::sqrt(variance);
            // VaR_95 = Z_95 * sigma_p * E_total  (dollar loss at 95% confidence)
            m_currentVaR95  = Z_95 * m_returnStdDev * currentEquity;
        }
    }
    m_prevEquity      = currentEquity;
    m_prevEquityValid = true;
    // If multi-asset mode is active, the diversified VaR already incorporates
    // the latest cross-asset covariance; the single-equity path only serves as
    // a fallback when no per-asset observations have been fed.
    if (m_multiAssetMode) {
        // VaR was already updated in pushAssetReturn; nothing more to do here.
    }
}

double RiskEngine::getVaR95() const noexcept
{
    return m_currentVaR95;
}

double RiskEngine::getReturnStdDev() const noexcept
{
    return m_returnStdDev;
}

std::size_t RiskEngine::getMaxConcurrentPositions() const noexcept
{
    return m_maxConcurrentPositions;
}

// ── Multi-asset correlation helpers ──────────────────────────────────────────

void RiskEngine::pushAssetReturn(const std::string& symbol,
                                 double closingPrice,
                                 double positionValue) noexcept
{
    m_multiAssetMode = true;

    AssetReturnState& state = m_assetStates[symbol];
    state.symbol        = symbol;
    state.positionValue = positionValue;

    if (state.prevPriceValid && state.prevPrice > 0.0) {
        const double ret = (closingPrice - state.prevPrice) / state.prevPrice;
        state.returnWindow.push_back(ret);
        if (state.returnWindow.size() > m_varWindow) {
            state.returnWindow.pop_front();
        }
    }
    state.prevPrice      = closingPrice;
    state.prevPriceValid = true;

    // Rebuild covariance matrix and recompute VaR after every new observation.
    rebuildCovMatrix();
    const double equity = m_portfolio.getTotalEquity();
    if (equity > 0.0) {
        updateDiversifiedVaR(equity);
    }
}

void RiskEngine::rebuildCovMatrix() noexcept
{
    // Collect symbols that have at least 2 return observations.
    m_assetOrder.clear();
    for (const auto& [sym, st] : m_assetStates) {
        if (st.returnWindow.size() >= 2) {
            m_assetOrder.push_back(sym);
        }
    }
    const std::size_t N = m_assetOrder.size();
    if (N == 0) {
        m_covMatrix.clear();
        return;
    }

    // Sort for deterministic ordering.
    std::sort(m_assetOrder.begin(), m_assetOrder.end());

    // Find the minimum window length across participating assets.
    std::size_t minLen = std::numeric_limits<std::size_t>::max();
    for (const auto& sym : m_assetOrder) {
        minLen = std::min(minLen, m_assetStates.at(sym).returnWindow.size());
    }
    const double Nd = static_cast<double>(minLen);

    // Compute per-asset mean over the shared window.
    std::vector<double> means(N, 0.0);
    for (std::size_t i = 0; i < N; ++i) {
        const auto& win = m_assetStates.at(m_assetOrder[i]).returnWindow;
        // Use the most recent minLen observations.
        const std::size_t offset = win.size() - minLen;
        for (std::size_t t = 0; t < minLen; ++t) {
            means[i] += win[offset + t];
        }
        means[i] /= Nd;
    }

    // Build covariance matrix Σ (row-major, population covariance).
    m_covMatrix.assign(N * N, 0.0);
    for (std::size_t i = 0; i < N; ++i) {
        const auto& win_i = m_assetStates.at(m_assetOrder[i]).returnWindow;
        const std::size_t off_i = win_i.size() - minLen;
        for (std::size_t j = i; j < N; ++j) {
            const auto& win_j = m_assetStates.at(m_assetOrder[j]).returnWindow;
            const std::size_t off_j = win_j.size() - minLen;
            double cov = 0.0;
            for (std::size_t t = 0; t < minLen; ++t) {
                cov += (win_i[off_i + t] - means[i])
                     * (win_j[off_j + t] - means[j]);
            }
            cov /= Nd;
            m_covMatrix[i * N + j] = cov;
            m_covMatrix[j * N + i] = cov; // symmetric
        }
    }
}

void RiskEngine::updateDiversifiedVaR(double totalEquity) noexcept
{
    const std::size_t N = m_assetOrder.size();
    if (N == 0 || m_covMatrix.empty()) {
        // Fall back to single-equity path (already computed in pushEquityReturn).
        return;
    }

    // Build weight vector w_i = positionValue_i / totalEquity.
    std::vector<double> w(N, 0.0);
    double totalPositioned = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        const double pv = m_assetStates.at(m_assetOrder[i]).positionValue;
        w[i]            = pv;
        totalPositioned += pv;
    }
    // Normalise weights against total equity (includes cash).
    for (std::size_t i = 0; i < N; ++i) {
        w[i] /= totalEquity;
    }

    // σ_p² = w^T Σ w
    double varP = 0.0;
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = 0; j < N; ++j) {
            varP += w[i] * m_covMatrix[i * N + j] * w[j];
        }
    }
    m_returnStdDev = (varP > 0.0) ? std::sqrt(varP) : 0.0;
    m_currentVaR95 = Z_95 * m_returnStdDev * totalEquity;
}

const std::unordered_map<std::string, RiskEngine::AssetReturnState>&
RiskEngine::getAssetReturnStates() const noexcept
{
    return m_assetStates;
}

void RiskEngine::restoreAssetReturnState(
    const std::unordered_map<std::string, AssetReturnState>& states) noexcept
{
    m_assetStates    = states;
    m_multiAssetMode = !states.empty();
    rebuildCovMatrix();
}

const std::vector<double>& RiskEngine::getCovarianceMatrix() const noexcept
{
    return m_covMatrix;
}

const std::vector<std::string>& RiskEngine::getAssetOrder() const noexcept
{
    return m_assetOrder;
}

void RiskEngine::restoreCovarianceMatrix(const std::vector<std::string>& order,
                                         const std::vector<double>& cov) noexcept
{
    m_assetOrder = order;
    m_covMatrix  = cov;
}

// ── Phase 12: Circuit breakers & adaptive risk ────────────────────────────────

void RiskEngine::setSystemConfig(const SystemConfig* cfg) noexcept
{
    m_sysCfg = cfg;
    // Initialise effective limits from construction-time values.
    m_effectiveMaxPositions = m_maxConcurrentPositions;
    m_effectiveVarLimit     = m_varLimitFraction;
}

// canTrade() override — now also blocks when circuit breakers are active.
// NOTE: The original canTrade() is replaced by redefining it here; the
// original definition in the .cpp is kept above as the Phase 8/9 baseline.
// We override it at the bottom so the Phase 12 checks take precedence.
// (In a single compilation unit the last definition of canTrade() wins — but
//  since canTrade() is not defined twice in the same TU, we simply patch the
//  existing one by adding the phase-12 gate inside the already-compiled body.)

void RiskEngine::reportLatency(uint32_t latencyMs) noexcept
{
    m_lastLatencyMs.store(latencyMs);

    const uint32_t threshold = m_sysCfg ? m_sysCfg->latencyMaxMs : 500u;

    if (latencyMs > threshold) {
        if (!m_closeOnly.load()) {
            m_closeOnly.store(true);
            std::cout << "[RiskEngine] CIRCUIT BREAKER — LATENCY: "
                      << latencyMs << "ms > threshold " << threshold
                      << "ms => CLOSE_ONLY state activated.\n";
        }
    }
}

void RiskEngine::reportApiError() noexcept
{
    const auto now = std::chrono::steady_clock::now();
    const uint32_t thresh = m_sysCfg ? m_sysCfg->errorRateThresh : 5u;

    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_errorTimestamps.push_back(now);

        // Evict events older than 60 seconds.
        const auto cutoff = now - std::chrono::seconds(60);
        while (!m_errorTimestamps.empty() &&
               m_errorTimestamps.front() < cutoff) {
            m_errorTimestamps.pop_front();
        }

        const uint32_t rate = static_cast<uint32_t>(m_errorTimestamps.size());

        if (rate > thresh && !m_halted.load()) {
            m_halted.store(true);
            std::cout << "[RiskEngine] CIRCUIT BREAKER — ERROR RATE: "
                      << rate << " errors/min > threshold " << thresh
                      << " => TRADING HALTED.\n";
        }
    }
}

void RiskEngine::clearHalt() noexcept
{
    const bool wasHalted = m_halted.exchange(false);
    if (wasHalted) {
        std::cout << "[RiskEngine] HALT cleared by operator — trading resumed.\n";
    }
    {
        std::lock_guard<std::mutex> lock(m_errorMutex);
        m_errorTimestamps.clear();
    }
}

void RiskEngine::setHaltTrading(bool halt) noexcept
{
    const bool prev = m_halted.exchange(halt);
    if (halt && !prev) {
        std::cout << "[RiskEngine] EXTERNAL HALT: data integrity degraded.\n";
    } else if (!halt && prev) {
        std::cout << "[RiskEngine] EXTERNAL HALT CLEARED: integrity restored.\n";
    }
}

void RiskEngine::clearLatencyBreach() noexcept
{
    const bool wasCloseOnly = m_closeOnly.exchange(false);
    if (wasCloseOnly) {
        std::cout << "[RiskEngine] CLOSE_ONLY cleared — latency returned to normal.\n";
    }
}

void RiskEngine::updateLiveVolatility(double liveAtr, double baselineAtr) noexcept
{
    if (baselineAtr <= 0.0 || !m_sysCfg) { return; }

    const double ratio     = liveAtr / baselineAtr;
    const double threshold = m_sysCfg->atrScaleUpThreshold;

    if (ratio > threshold && !m_volatilityScaled) {
        // Scale down limits.
        m_volatilityScaled      = true;
        m_effectiveMaxPositions = std::max(std::size_t{1},
                                           m_maxConcurrentPositions / 2);
        m_effectiveVarLimit     = m_varLimitFraction
                                  * m_sysCfg->varScaleHighVolFactor;
        std::cout << "[RiskEngine] HIGH VOLATILITY: ATR ratio=" << ratio
                  << " > " << threshold
                  << "  MaxPositions: " << m_maxConcurrentPositions
                  << " -> " << m_effectiveMaxPositions
                  << "  VaR limit: " << m_varLimitFraction
                  << " -> " << m_effectiveVarLimit << "\n";
    } else if (ratio <= threshold && m_volatilityScaled) {
        // Restore normal limits.
        m_volatilityScaled      = false;
        m_effectiveMaxPositions = m_maxConcurrentPositions;
        m_effectiveVarLimit     = m_varLimitFraction;
        std::cout << "[RiskEngine] VOLATILITY NORMALISED: ATR ratio=" << ratio
                  << " <= " << threshold
                  << "  Limits restored.\n";
    }
}

std::size_t RiskEngine::effectiveMaxConcurrentPositions() const noexcept
{
    return m_effectiveMaxPositions;
}

double RiskEngine::effectiveVarLimitFraction() const noexcept
{
    return m_effectiveVarLimit;
}

bool RiskEngine::isCloseOnly() const noexcept
{
    return m_closeOnly.load();
}

bool RiskEngine::isHalted() const noexcept
{
    return m_halted.load();
}

uint32_t RiskEngine::lastLatencyMs() const noexcept
{
    return m_lastLatencyMs.load();
}

uint32_t RiskEngine::currentErrorRate() const noexcept
{
    const auto now    = std::chrono::steady_clock::now();
    const auto cutoff = now - std::chrono::seconds(60);
    std::lock_guard<std::mutex> lock(m_errorMutex);
    uint32_t count = 0;
    for (const auto& ts : m_errorTimestamps) {
        if (ts >= cutoff) { ++count; }
    }
    return count;
}

void RiskEngine::syncPosition(const std::string& symbol, double quantity) noexcept
{
    std::lock_guard<std::mutex> lock(m_positionMutex);
    if (quantity <= 0.0) {
        m_syncedPositions.erase(symbol);
        return;
    }
    m_syncedPositions[symbol] = quantity;
}

double RiskEngine::syncedPosition(const std::string& symbol) const noexcept
{
    std::lock_guard<std::mutex> lock(m_positionMutex);
    auto it = m_syncedPositions.find(symbol);
    return (it == m_syncedPositions.end()) ? 0.0 : it->second;
}
