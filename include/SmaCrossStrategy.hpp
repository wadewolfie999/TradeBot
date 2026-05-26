#pragma once
#include "IStrategy.hpp"
#include <cstddef>
#include <deque>
#include <string>

// Default strategy identifier.
inline constexpr char SMA_DEFAULT_ID[] = "SMA_01";

class SmaCrossStrategy : public IStrategy {
public:
    SmaCrossStrategy(std::size_t fast, std::size_t slow,
                     std::string strategyId = SMA_DEFAULT_ID);

    // Phase 10: primary AlphaSignal interface.
    // Returns conviction = +1.0 (golden cross), -1.0 (death cross), 0.0 (none).
    AlphaSignal generateSignal(const MarketCandle& candle) override;

    // Legacy shim for backward compatibility (delegates to generateSignal).
    Signal onCandle(const MarketCandle& candle) override;

    // Returns the most recent ATR(atrPeriod) value.
    // Returns 0.0 if fewer than atrPeriod+1 candles have been seen.
    double getATR() const noexcept;

    // True once at least atrPeriod+1 candles have been processed and ATR is valid.
    bool isATRValid() const noexcept;

    // Read the strategy identifier assigned at construction.
    const std::string& getStrategyId() const noexcept;

private:
    std::string m_strategyId;
    std::size_t m_fastPeriod;
    std::size_t m_slowPeriod;

    std::deque<double> m_priceBuffer;

    double m_prevFastSma{0.0};
    double m_prevSlowSma{0.0};
    bool   m_prevValid{false};

    static double calcSma(const std::deque<double>& buf, std::size_t period);

    // ATR state
    static constexpr std::size_t ATR_PERIOD = 14;
    std::deque<double> m_trueRanges;  // rolling true-range buffer (size <= ATR_PERIOD)
    double             m_prevClose{0.0};
    bool               m_prevCloseValid{false};
    double             m_currentATR{0.0};
    bool               m_atrValid{false};
};
