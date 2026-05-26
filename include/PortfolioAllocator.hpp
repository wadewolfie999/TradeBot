#pragma once
#include "IStrategy.hpp"
#include "RegimeDetector.hpp"
#include <atomic>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// ── Phase 10 / Phase 11: PortfolioAllocator ───────────────────────────────────
//
// Phase 10 — static weighted ensemble:
//   S_total = sum_i( W_i * S_i )
//   Gate thresholds: S_total >= +0.3 -> BUY, <= -0.3 -> SELL, else NONE
//
// Phase 11 — dynamic, regime-aware weighting:
//   W_i(t) = lambda_regime * M(i, K_t) + lambda_perf * Phi_i(t)
//
//   where:
//     K_t       = current MarketRegime from RegimeDetector
//     M(i, K_t) = affinity of strategy i for regime K_t
//     Phi_i(t)  = rolling normalised performance score for strategy i
//                 (rolling profit-factor mapped to [-1, 1] over last 20 trades)
//     lambda_regime = 0.7
//     lambda_perf   = 0.3
//
//   Weights are normalised so sum(|W_i|) <= 1.0 before ensembling.

// ─── Allocation result forwarded to ExecutionEngine ───────────────────────────

struct AllocationResult {
    std::string symbol;
    Signal      action{Signal::NONE};
    double      totalConviction{0.0};
    std::string dominantStrategyId;
    MarketRegime regime{MarketRegime::CHAOS};  // regime at time of decision
};

// ─── PortfolioAllocator ───────────────────────────────────────────────────────

class PortfolioAllocator {
public:
    static constexpr double LAMBDA_REGIME = 0.7;
    static constexpr double LAMBDA_PERF   = 0.3;
    static constexpr int    PHI_WINDOW    = 20;  // rolling trade window for Phi

    // longThreshold  — S_total threshold for BUY  (default 0.3)
    // shortThreshold — S_total threshold for SELL (default 0.3)
    explicit PortfolioAllocator(double longThreshold  = 0.3,
                                double shortThreshold = 0.3);

    // ── Phase 10 static weight API (backward-compat; ignored in Phase 11 mode) ──
    void   setWeight(const std::string& strategyId, double weight);
    double getWeight(const std::string& strategyId) const noexcept;

    // ── Phase 11 dynamic weight API ──────────────────────────────────────────
    // Attach a RegimeDetector. When set, dynamic weighting is used.
    void setRegimeDetector(RegimeDetector* rd) noexcept { m_regimeDetector = rd; }

    // Called by AnalyticsEngine telemetry on every closed trade.
    // strategyId: the owning strategy; realizedPnL: net PnL of the closed round-trip.
    void onTradeClosed(const std::string& strategyId, double realizedPnL);

    // Compute dynamic W_i(t) for a given strategy given current regime.
    // Returns a clamped weight.
    double computeDynamicWeight(const std::string& strategyId,
                                MarketRegime regime) const noexcept;

    // Current rolling performance score Phi_i(t) for a strategy [-1, 1].
    double getPhi(const std::string& strategyId) const noexcept;

    // Recompute and cache dynamic strategy weights after a regime shift.
    // Designed to be invoked asynchronously from live ingestion callbacks.
    void recomputeWeights(MarketRegime regime);
    uint64_t recomputeCount() const noexcept;

    // Ensemble a batch of AlphaSignals for a single asset / timestep.
    AllocationResult ensemble(const std::vector<AlphaSignal>& signals) const;

    // ── Serialisation helpers ─────────────────────────────────────────────
    struct PhiState {
        std::string        strategyId;
        std::deque<double> pnlWindow;   // last PHI_WINDOW closed-trade PnLs
        double             phi{0.0};    // current score
    };

    const std::unordered_map<std::string, PhiState>& getPhiStates() const noexcept {
        return m_phiStates;
    }
    void restorePhiStates(const std::unordered_map<std::string, PhiState>& s) {
        m_phiStates = s;
    }

private:
    std::unordered_map<std::string, double>   m_weights;       // Phase 10 static
    std::unordered_map<std::string, PhiState> m_phiStates;     // Phase 11 rolling perf
    std::unordered_map<std::string, double>   m_cachedDynamicWeights;

    double          m_longThreshold;
    double          m_shortThreshold;
    RegimeDetector* m_regimeDetector{nullptr};
    mutable std::mutex m_mutex;
    std::atomic<uint64_t> m_recomputeCount{0};

    // Recompute Phi for a strategy from its pnlWindow.
    void recomputePhi(PhiState& ps) const noexcept;
};
