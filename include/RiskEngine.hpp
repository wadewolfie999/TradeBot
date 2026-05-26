#pragma once
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations.
class PortfolioManager;
struct SystemConfig;

// RiskEngine enforces structural risk limits from RISK_MODEL_DRAFT.md.
//
// Phase 7 additions:
//   - MaxConcurrentPositions: hard cap on how many symbols may be held at once.
//     canTrade() returns false when the cap is already reached, blocking new
//     entries without affecting existing-position closes.
//   - The PortfolioManager reference is injected at construction so the engine
//     can query the live open-position count on every gate evaluation.
//
// Phase 8 additions (Workstream 3.3):
//   - Parametric VaR: maintains a rolling window of per-candle portfolio equity
//     returns and computes the 95 % confidence VaR:
//       VaR_95 = 1.645 * sigma_p * E_total
//     canTrade() blocks new entries if VaR_95 exceeds varLimitFraction of
//     total equity (default: 5 %).
//   - pushEquityReturn(): call once per candle with the latest equity value.
//   - getVaR95(): returns the current dollar-VaR at 95 % confidence.
class RiskEngine {
    // ── Phase 9 – Correlation-Adjusted Risk Model ─────────────────────────
    // Each actively-traded symbol contributes a rolling window of per-candle
    // *price* returns.  The full covariance matrix Σ is rebuilt whenever any
    // symbol pushes a new observation.
    //
    // Portfolio volatility:
    //   σ_p = sqrt( w^T Σ w )
    // where w_i = (position_value_i / total_equity).
    //
    // Diversified VaR:
    //   VaR_95 = Z_95 * σ_p * E_total
    //
    // The gate blocks new entries when diversified VaR_95 > varLimitFraction
    // × E_total (same threshold as Phase 8, now correlation-aware).
    //
    // pushAssetReturn() should be called once per candle per symbol (after
    // updatePnL) with the symbol name and its closing price.
    // pushEquityReturn() retains its Phase 8 role as a fallback / legacy path
    // when only a single undifferentiated equity stream is available, but in
    // multi-asset mode the correlation path dominates.
    // ─────────────────────────────────────────────────────────────────────
    //
    // Phase 12 additions (Workstream 7.4) — Adaptive Risk-On-The-Fly:
    //   - reportLatency(): called by LiveDataAdapter after each ping/pong or REST
    //     round-trip.  If latencyMs > latencyMaxMs (default 500 ms) the engine
    //     enters CLOSE_ONLY state — canTrade() returns false until latency clears.
    //   - reportApiError(): called by BrokerGateway on each API error.  A sliding
    //     1-minute window tracks the error rate; if it exceeds errorRateThresh the
    //     engine enters HALTED state — canTrade() returns false indefinitely until
    //     clearHalt() is explicitly called.
    //   - updateLiveVolatility(): fed the latest streaming ATR value.  When ATR
    //     exceeds atrScaleUpThreshold × baseline ATR, MaxConcurrentPositions and
    //     VaR limit are scaled down dynamically without restarting the system.
    // ─────────────────────────────────────────────────────────────────────
public:
    // `maxConcurrentPositions` = 0 means unlimited (disabled).
    // `varLimitFraction`        = fraction of equity the VaR gate allows
    //                             (e.g. 0.05 = 5 %).  Set to 0 to disable.
    // `varWindow`               = rolling return window length (default: 252).
    explicit RiskEngine(const PortfolioManager& portfolio,
                        std::size_t maxConcurrentPositions = 2,
                        double      varLimitFraction       = 0.05,
                        std::size_t varWindow              = 252) noexcept;

    // Phase 12: inject the SystemConfig so circuit-breaker thresholds can be
    // read without hardcoding them inside RiskEngine.
    void setSystemConfig(const SystemConfig* cfg) noexcept;

    // Returns false when any hard limit is breached (drawdown OR max positions
    // OR circuit breakers).
    // Must return true before a new BUY order can be submitted.
    bool canTrade() const noexcept;

    // Update the current total drawdown (as a fraction, e.g. 0.05 == 5%).
    void setTotalDrawdown(double drawdown) noexcept;

    // Update the current daily drawdown (as a fraction, e.g. 0.02 == 2%).
    void setDailyDrawdown(double drawdown) noexcept;

    // Feed the latest total-equity value so the rolling-return window stays
    // current.  Call once per candle (after updatePnL).
    void pushEquityReturn(double currentEquity) noexcept;

    // Feed the latest closing price for `symbol` so the per-asset rolling
    // return window stays current.  Call once per candle per symbol.
    // `positionValue` is the current mark-to-market value of the position in
    // that symbol (used to compute the weight vector w).
    void pushAssetReturn(const std::string& symbol,
                         double closingPrice,
                         double positionValue) noexcept;

    // ── Phase 12: Circuit breakers & adaptive scaling ─────────────────────

    // Report a WSS/REST latency sample (milliseconds).  If latencyMs exceeds
    // the configured threshold, the engine enters CLOSE_ONLY state.
    void reportLatency(uint32_t latencyMs) noexcept;

    // Report one API error (connection drop, HTTP 5xx, etc.).  The error is
    // logged to a 1-minute sliding window; if the rate exceeds errorRateThresh
    // the engine enters HALTED state.
    void reportApiError() noexcept;

    // Clear the HALTED state after operator review (manual reset).
    void clearHalt() noexcept;

    // Explicit external halt toggle used by live data integrity guards.
    void setHaltTrading(bool halt) noexcept;

    // Clear the CLOSE_ONLY latency state (called when latency returns below
    // threshold for a sustained period).
    void clearLatencyBreach() noexcept;

    // Feed the latest ATR value and a baseline ATR (computed at session start
    // or over a long rolling window).  Dynamically scales position cap and VaR
    // limit when liveAtr > baselineAtr * atrScaleUpThreshold.
    void updateLiveVolatility(double liveAtr, double baselineAtr) noexcept;

    // Current adaptive effective cap (may differ from construction-time cap
    // when volatility scaling is active).
    std::size_t effectiveMaxConcurrentPositions() const noexcept;

    // Current adaptive effective VaR limit fraction.
    double effectiveVarLimitFraction() const noexcept;

    // True when the latency circuit breaker is active (CLOSE_ONLY).
    bool isCloseOnly() const noexcept;

    // True when the error-rate circuit breaker has halted all trading.
    bool isHalted() const noexcept;

    // Latest latency sample recorded.
    uint32_t lastLatencyMs() const noexcept;

    // Current error count within the 1-minute sliding window.
    uint32_t currentErrorRate() const noexcept;

    // Synchronize current live position quantity per symbol from broker fills.
    void syncPosition(const std::string& symbol, double quantity) noexcept;
    double syncedPosition(const std::string& symbol) const noexcept;

    // Current parametric VaR at 95 % confidence (dollar amount).
    // In multi-asset mode this reflects correlation-adjusted σ_p.
    // Returns 0.0 until the rolling window contains at least 2 samples.
    double getVaR95() const noexcept;

    // Current rolling standard deviation of equity returns (fraction).
    double getReturnStdDev() const noexcept;

    // Read the configured concurrent-position cap.
    std::size_t getMaxConcurrentPositions() const noexcept;

    // ── Serialization helpers (used by StateSerializer) ───────────────────
    struct AssetReturnState {
        std::string            symbol;
        std::deque<double>     returnWindow;  // per-candle fractional returns
        double                 prevPrice{0.0};
        bool                   prevPriceValid{false};
        double                 positionValue{0.0}; // latest w_i numerator
    };

    // Read-only access to the per-asset return state (for serialisation).
    const std::unordered_map<std::string, AssetReturnState>&
        getAssetReturnStates() const noexcept;

    // Restore per-asset return state from a snapshot (used by --resume path).
    void restoreAssetReturnState(
        const std::unordered_map<std::string, AssetReturnState>& states) noexcept;

    // Read-only access to the covariance matrix (row-major, symbols ordered
    // by m_assetOrder).  Exposed for diagnostics and serialisation.
    const std::vector<double>& getCovarianceMatrix() const noexcept;

    // Symbols in the order used by the covariance matrix rows/columns.
    const std::vector<std::string>& getAssetOrder() const noexcept;

    // Restore covariance matrix and asset order from snapshot.
    void restoreCovarianceMatrix(const std::vector<std::string>& order,
                                 const std::vector<double>& cov) noexcept;

private:
    // Hard limits (fractional): from RISK_MODEL_DRAFT.md.
    static constexpr double MAX_TOTAL_DRAWDOWN = 0.10; // 10%
    static constexpr double MAX_DAILY_DRAWDOWN = 0.04; // 4%
    static constexpr double Z_95               = 1.645; // Z-score for 95% VaR

    double m_totalDrawdown{0.0};
    double m_dailyDrawdown{0.0};

    const PortfolioManager& m_portfolio;
    std::size_t m_maxConcurrentPositions;

    // VaR state
    double      m_varLimitFraction{0.05};
    std::size_t m_varWindow{252};
    double      m_prevEquity{0.0};
    bool        m_prevEquityValid{false};
    std::deque<double> m_returnWindow;
    double      m_currentVaR95{0.0};
    double      m_returnStdDev{0.0};

    // ── Multi-asset correlation state ─────────────────────────────────────
    // Per-symbol rolling return window (length capped at m_varWindow).
    std::unordered_map<std::string, AssetReturnState> m_assetStates;

    // Cached covariance matrix in row-major layout.
    // Size = N*N where N = m_assetOrder.size().
    std::vector<double>      m_covMatrix;
    std::vector<std::string> m_assetOrder;  // index <-> symbol mapping

    // Rebuild the covariance matrix from current m_assetStates windows.
    // Called whenever any asset pushes a new return observation.
    void rebuildCovMatrix() noexcept;

    // Compute diversified portfolio volatility σ_p given the current
    // covariance matrix and position-value weights.
    // Sets m_returnStdDev and m_currentVaR95.
    // totalEquity must be > 0.
    void updateDiversifiedVaR(double totalEquity) noexcept;

    // True once the multi-asset path is active (>=1 asset pushed via
    // pushAssetReturn).  When false, falls back to Phase-8 single-asset path.
    bool m_multiAssetMode{false};

    // ── Phase 12: Circuit breaker state ───────────────────────────────────
    // Pointer to SystemConfig (optional; nullptr in BACKTEST mode).
    const SystemConfig* m_sysCfg{nullptr};

    // Latency circuit breaker.
    std::atomic<bool>     m_closeOnly{false};
    std::atomic<uint32_t> m_lastLatencyMs{0};

    // Error-rate circuit breaker.
    std::atomic<bool>     m_halted{false};
    mutable std::mutex    m_errorMutex;
    std::deque<std::chrono::steady_clock::time_point> m_errorTimestamps;
    std::unordered_map<std::string, double> m_syncedPositions;
    mutable std::mutex    m_positionMutex;

    // Adaptive scaling state.
    std::size_t m_effectiveMaxPositions{0};  // mirrors m_maxConcurrentPositions initially
    double      m_effectiveVarLimit{0.0};    // mirrors m_varLimitFraction initially
    bool        m_volatilityScaled{false};   // true when high-vol scaling is active
};
