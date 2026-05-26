#pragma once
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

// ── Phase 11: RegimeDetector ──────────────────────────────────────────────────
//
// Classifies the current market into one of three regimes every time a new
// price observation arrives:
//
//   K_trend  — ADX > 25  AND low rolling variance
//   K_range  — ADX < 20  AND stable boundary oscillation (variance <= varThreshold)
//   K_chaos  — everything else (high ADX AND / OR high variance)
//
// ADX is computed from a Wilder-smoothed 14-period average (Wilder DMI).
// Variance is the rolling population variance of close-price fractional returns
// over a configurable window (default 20).
//
// Affinity matrix  M(strategy, regime):
//   M(SMA_01, K_trend) = +1.0    SMA_01 thrives in trends
//   M(SMA_01, K_range) = -0.5    SMA_01 whipsaws in ranges
//   M(MR_01,  K_range) = +1.0    Mean-reversion thrives in ranges
//   M(MR_01,  K_trend) = -0.5    Mean-reversion fights trends
//   M(any,    K_chaos) =  0.0    Risk-off in chaos

enum class MarketRegime { TREND, RANGE, CHAOS };

class RegimeDetector {
public:
    // adxPeriod      — Wilder smoothing period for DI+ / DI- / ADX (default 14)
    // varWindow      — look-back window for variance computation (default 20)
    // varThreshold   — variance above this -> chaos candidate (default 1e-4)
    // adxTrendAbove  — ADX > this -> trend-candidate (default 25)
    // adxRangeBelow  — ADX < this -> range-candidate (default 20)
    explicit RegimeDetector(int    adxPeriod     = 14,
                            int    varWindow      = 20,
                            double varThreshold   = 1e-4,
                            double adxTrendAbove  = 25.0,
                            double adxRangeBelow  = 20.0) noexcept;

    // Feed a new candle's OHLC data.  Returns the updated regime.
    MarketRegime update(double high, double low, double close) noexcept;

    MarketRegime currentRegime()  const noexcept { return m_regime;   }
    double       currentADX()     const noexcept { return m_adx;      }
    double       currentVariance()const noexcept { return m_variance; }

    static const char* regimeName(MarketRegime r) noexcept;

    // Affinity M(strategy_id, regime) — returns 0.0 for unknown strategies.
    static double affinity(const std::string& sid, MarketRegime r) noexcept;

    // ── Serialisation helpers ─────────────────────────────────────────────
    struct State {
        // Wilder ADX internals
        double smoothTR{0.0};
        double smoothDMPlus{0.0};
        double smoothDMMinus{0.0};
        double adx{0.0};
        int    adxObservations{0};

        // Variance internals
        std::deque<double> returnWindow;
        double             prevClose{0.0};
        bool               prevCloseValid{false};
        double             variance{0.0};

        // Previous candle HL for DM computation
        double prevHigh{0.0};
        double prevLow{0.0};
        bool   prevHLValid{false};

        MarketRegime regime{MarketRegime::CHAOS};
    };

    const State& getState()                  const noexcept { return m_state;  }
    void         restoreState(const State& s)      noexcept;

private:
    int    m_adxPeriod;
    int    m_varWindow;
    double m_varThreshold;
    double m_adxTrendAbove;
    double m_adxRangeBelow;

    State        m_state;
    MarketRegime m_regime{MarketRegime::CHAOS};
    double       m_adx{0.0};
    double       m_variance{0.0};

    void classifyRegime() noexcept;
};
