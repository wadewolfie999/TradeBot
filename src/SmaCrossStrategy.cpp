#include "SmaCrossStrategy.hpp"
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <algorithm>
#include <cmath>

SmaCrossStrategy::SmaCrossStrategy(std::size_t fast, std::size_t slow,
                                   std::string strategyId)
    : m_strategyId(std::move(strategyId))
    , m_fastPeriod(fast)
    , m_slowPeriod(slow)
{
    if (fast == 0 || slow == 0) {
        throw std::invalid_argument("SMA periods must be > 0");
    }
    if (fast >= slow) {
        throw std::invalid_argument("Fast period must be less than slow period");
    }
}

double SmaCrossStrategy::calcSma(const std::deque<double>& buf, std::size_t period)
{
    // Sum the last `period` elements from the back of the deque.
    auto begin = buf.end() - static_cast<std::ptrdiff_t>(period);
    double sum = std::accumulate(begin, buf.end(), 0.0);
    return sum / static_cast<double>(period);
}

// ── Phase 10: generateSignal ──────────────────────────────────────────────────

AlphaSignal SmaCrossStrategy::generateSignal(const MarketCandle& candle)
{
    m_priceBuffer.push_back(candle.close);

    // Cap the buffer to slow period (we only need the last slowPeriod prices).
    if (m_priceBuffer.size() > m_slowPeriod) {
        m_priceBuffer.pop_front();
    }

    // ── ATR update (Wilder's smoothed ATR) ──────────────────────────────
    if (m_prevCloseValid) {
        const double trueRange = std::max({
            candle.high - candle.low,
            std::abs(candle.high - m_prevClose),
            std::abs(candle.low  - m_prevClose)
        });
        m_trueRanges.push_back(trueRange);
        if (m_trueRanges.size() > ATR_PERIOD) {
            m_trueRanges.pop_front();
        }
        if (m_trueRanges.size() == ATR_PERIOD) {
            if (!m_atrValid) {
                // Seed: simple average of first ATR_PERIOD true ranges.
                m_currentATR = std::accumulate(m_trueRanges.begin(),
                                               m_trueRanges.end(), 0.0)
                               / static_cast<double>(ATR_PERIOD);
                m_atrValid = true;
            } else {
                // Wilder's smoothing: ATR = (prev*(n-1) + TR) / n
                m_currentATR = (m_currentATR * (ATR_PERIOD - 1) + trueRange)
                               / static_cast<double>(ATR_PERIOD);
            }
        }
    }
    m_prevClose      = candle.close;
    m_prevCloseValid = true;

    // We need at least slowPeriod prices before we can compute both SMAs.
    if (m_priceBuffer.size() < m_slowPeriod) {
        return {candle.symbol, m_strategyId, 0.0};
    }

    const double fastSma = calcSma(m_priceBuffer, m_fastPeriod);
    const double slowSma = calcSma(m_priceBuffer, m_slowPeriod);

    double conviction = 0.0;
    if (m_prevValid) {
        // Crossover: fast crosses above slow -> strong long conviction
        if (fastSma > slowSma && m_prevFastSma <= m_prevSlowSma) {
            conviction = 1.0;
            std::cout << "[SIGNAL] " << m_strategyId << " LONG (+1.0)"
                      << " | Timestamp: " << candle.epochTimestamp
                      << " | Price: "     << candle.close << "\n";
        }
        // Crossunder: fast crosses below slow -> strong bearish conviction
        else if (fastSma < slowSma && m_prevFastSma >= m_prevSlowSma) {
            conviction = -1.0;
            std::cout << "[SIGNAL] " << m_strategyId << " SHORT (-1.0)"
                      << " | Timestamp: " << candle.epochTimestamp
                      << " | Price: "     << candle.close << "\n";
        }
    }

    m_prevFastSma = fastSma;
    m_prevSlowSma = slowSma;
    m_prevValid   = true;
    return {candle.symbol, m_strategyId, conviction};
}

Signal SmaCrossStrategy::onCandle(const MarketCandle& candle)
{
    return alphaToSignal(generateSignal(candle));
}

double SmaCrossStrategy::getATR() const noexcept
{
    return m_currentATR;
}

bool SmaCrossStrategy::isATRValid() const noexcept
{
    return m_atrValid;
}

const std::string& SmaCrossStrategy::getStrategyId() const noexcept
{
    return m_strategyId;
}
