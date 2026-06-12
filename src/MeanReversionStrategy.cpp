#include "MeanReversionStrategy.hpp"
#include <iostream>
#include <numeric>
#include <cmath>
#include <iomanip>
#include <algorithm>

MeanReversionStrategy::MeanReversionStrategy(std::size_t bbPeriod,
                                             double      bbStdDevMult,
                                             std::size_t rsiPeriod,
                                             std::string strategyId)
    : m_strategyId(std::move(strategyId))
    , m_bbPeriod(bbPeriod)
    , m_bbStdDevMult(bbStdDevMult)
    , m_rsiPeriod(rsiPeriod)
{}

// ── Static helpers ────────────────────────────────────────────────────────────

double MeanReversionStrategy::calcMean(const std::deque<double>& buf) noexcept
{
    if (buf.empty()) { return 0.0; }
    return std::accumulate(buf.begin(), buf.end(), 0.0)
           / static_cast<double>(buf.size());
}

double MeanReversionStrategy::calcStdDev(const std::deque<double>& buf,
                                         double mean) noexcept
{
    if (buf.size() < 2) { return 0.0; }
    double sumSq = 0.0;
    for (const double v : buf) {
        const double diff = v - mean;
        sumSq += diff * diff;
    }
    return std::sqrt(sumSq / static_cast<double>(buf.size()));
}

// ── Phase 10: generateSignal ──────────────────────────────────────────────────

AlphaSignal MeanReversionStrategy::generateSignal(const MarketCandle& candle)
{
    const double close = candle.close;

    // ── Bollinger Band update ─────────────────────────────────────────────
    m_priceBuffer.push_back(close);
    if (m_priceBuffer.size() > m_bbPeriod) {
        m_priceBuffer.pop_front();
    }

    // ── RSI update (Wilder's EMA smoothing) ──────────────────────────────
    if (m_prevCloseValid) {
        const double change = close - m_prevClose;
        const double gain   = (change > 0.0) ? change : 0.0;
        const double loss   = (change < 0.0) ? -change : 0.0;

        if (!m_rsiSeeded) {
            // Accumulate seed bars
            m_seedGainSum += gain;
            m_seedLossSum += loss;
            ++m_rsiSeedCount;

            if (m_rsiSeedCount == static_cast<int>(m_rsiPeriod)) {
                m_avgGain  = m_seedGainSum / static_cast<double>(m_rsiPeriod);
                m_avgLoss  = m_seedLossSum / static_cast<double>(m_rsiPeriod);
                m_rsiSeeded = true;
            }
        } else {
            // Wilder's smoothing: avgGain = (prev*(n-1) + gain) / n
            m_avgGain = (m_avgGain * (m_rsiPeriod - 1) + gain)
                        / static_cast<double>(m_rsiPeriod);
            m_avgLoss = (m_avgLoss * (m_rsiPeriod - 1) + loss)
                        / static_cast<double>(m_rsiPeriod);
        }
    }
    m_prevClose      = close;
    m_prevCloseValid = true;

    // ── Check readiness ───────────────────────────────────────────────────
    m_ready = (m_priceBuffer.size() == m_bbPeriod && m_rsiSeeded);
    if (!m_ready) {
        return {candle.symbol, m_strategyId, 0.0};
    }

    // ── Bollinger Band conviction ─────────────────────────────────────────
    const double bbMean   = calcMean(m_priceBuffer);
    const double bbStdDev = calcStdDev(m_priceBuffer, bbMean);

    double bbConviction = 0.0;
    if (bbStdDev > 0.0) {
        const double upperBand = bbMean + m_bbStdDevMult * bbStdDev;
        const double lowerBand = bbMean - m_bbStdDevMult * bbStdDev;
        const double bandWidth = upperBand - lowerBand;

        if (bandWidth > 0.0) {
            // pB: 0.0 = at lower band (oversold), 1.0 = at upper band (overbought)
            const double pB = (close - lowerBand) / bandWidth;
            // bbConviction: +1.0 = maximally oversold, -1.0 = maximally overbought
            bbConviction = std::clamp(1.0 - 2.0 * pB, -1.0, 1.0);
        }
    }

    // ── RSI conviction ────────────────────────────────────────────────────
    double rsiConviction = 0.0;
    if (m_avgLoss > 0.0) {
        const double rs  = m_avgGain / m_avgLoss;
        const double rsi = 100.0 - (100.0 / (1.0 + rs));
        // Normalise: rsi=30 (oversold) -> +0.4, rsi=70 (overbought) -> -0.4
        // rsiNorm ∈ [-1, +1]; negated because high RSI = overbought (bearish for MR)
        const double rsiNorm = (rsi - 50.0) / 50.0;
        rsiConviction = -rsiNorm;
    } else if (m_avgGain > 0.0) {
        // avgLoss == 0: RSI = 100 (extreme overbought) -> bearish MR conviction
        rsiConviction = -1.0;
    }

    // ── Combine: 50% Bollinger Band + 50% RSI ────────────────────────────
    const double conviction = std::clamp(0.5 * bbConviction + 0.5 * rsiConviction,
                                         -1.0, 1.0);

    if (std::abs(conviction) >= 0.1) {
        std::cout << "[SIGNAL] " << m_strategyId
                  << (conviction > 0 ? " LONG (" : " SHORT (")
                  << std::fixed << std::setprecision(4) << conviction << ")"
                  << " | Timestamp: " << candle.epochTimestamp
                  << " | Price: "     << candle.close << "\n";
    }

    return {candle.symbol, m_strategyId, conviction};
}

Signal MeanReversionStrategy::onCandle(const MarketCandle& candle)
{
    return alphaToSignal(generateSignal(candle));
}

bool MeanReversionStrategy::isReady() const noexcept
{
    return m_ready;
}

const std::string& MeanReversionStrategy::getStrategyId() const noexcept
{
    return m_strategyId;
}
