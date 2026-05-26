#pragma once
#include "IStrategy.hpp"
#include <cstddef>
#include <deque>
#include <string>

// Default strategy identifier.
inline constexpr char MR_DEFAULT_ID[] = "MR_01";

// ── Phase 10: MeanReversionStrategy ──────────────────────────────────────────
//
// Generates a continuous conviction score S ∈ [-1.0, 1.0] based on the price
// position relative to Bollinger Bands (BB) combined with a normalised RSI.
//
// Bollinger Bands:
//   Middle Band (MB) = SMA(close, bbPeriod)
//   Upper Band  (UB) = MB + bbStdDevMult * StdDev(close, bbPeriod)
//   Lower Band  (LB) = MB - bbStdDevMult * StdDev(close, bbPeriod)
//
// %B position:   pB = (close - LB) / (UB - LB)  (0.0 = at LB, 1.0 = at UB)
//
// RSI (14-bar):  rsi ∈ [0, 100]
//   normalised:  rsiNorm = (rsi - 50) / 50  (range [-1, +1])
//
// Combined conviction:
//   bbConviction = 1.0 - 2.0 * pB        (oversold=+1, overbought=-1)
//   conviction   = 0.5 * bbConviction + 0.5 * (-rsiNorm)
//
// Sign convention (same as SMA strategy):
//   +1.0 = maximum long / oversold conviction
//   -1.0 = maximum short / overbought conviction

class MeanReversionStrategy : public IStrategy {
public:
    MeanReversionStrategy(std::size_t bbPeriod      = 20,
                          double      bbStdDevMult   = 2.0,
                          std::size_t rsiPeriod      = 14,
                          std::string strategyId     = MR_DEFAULT_ID);

    // Phase 10: primary AlphaSignal interface.
    AlphaSignal generateSignal(const MarketCandle& candle) override;

    // Legacy shim (delegates to generateSignal).
    Signal onCandle(const MarketCandle& candle) override;

    // True once enough bars have been seen to produce valid indicators.
    bool isReady() const noexcept;

    const std::string& getStrategyId() const noexcept;

private:
    std::string m_strategyId;
    std::size_t m_bbPeriod;
    double      m_bbStdDevMult;
    std::size_t m_rsiPeriod;

    std::deque<double> m_priceBuffer;   // rolling close prices for BB

    // RSI state (Wilder's smoothed)
    double m_avgGain{0.0};
    double m_avgLoss{0.0};
    double m_prevClose{0.0};
    bool   m_prevCloseValid{false};
    bool   m_rsiSeeded{false};
    int    m_rsiSeedCount{0};
    double m_seedGainSum{0.0};
    double m_seedLossSum{0.0};
    bool   m_ready{false};

    // Helpers
    static double calcMean(const std::deque<double>& buf) noexcept;
    static double calcStdDev(const std::deque<double>& buf, double mean) noexcept;
};
