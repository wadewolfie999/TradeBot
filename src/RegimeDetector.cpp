#include "RegimeDetector.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>

RegimeDetector::RegimeDetector(int adxPeriod, int varWindow,
                                double varThreshold,
                                double adxTrendAbove,
                                double adxRangeBelow) noexcept
    : m_adxPeriod(adxPeriod)
    , m_varWindow(varWindow)
    , m_varThreshold(varThreshold)
    , m_adxTrendAbove(adxTrendAbove)
    , m_adxRangeBelow(adxRangeBelow)
{}

// ── Wilder-smoothed ADX + rolling variance ────────────────────────────────────
MarketRegime RegimeDetector::update(double high, double low, double close) noexcept
{
    State& s = m_state;

    // ── 1. Variance of close-price fractional returns ─────────────────────
    if (s.prevCloseValid && s.prevClose > 0.0) {
        const double ret = (close - s.prevClose) / s.prevClose;
        s.returnWindow.push_back(ret);
        if (static_cast<int>(s.returnWindow.size()) > m_varWindow) {
            s.returnWindow.pop_front();
        }
        if (s.returnWindow.size() >= 2) {
            const double n = static_cast<double>(s.returnWindow.size());
            double mean = 0.0;
            for (double r : s.returnWindow) { mean += r; }
            mean /= n;
            double sq = 0.0;
            for (double r : s.returnWindow) { sq += (r - mean) * (r - mean); }
            s.variance = sq / n;
        }
    }
    s.prevClose      = close;
    s.prevCloseValid = true;
    m_variance       = s.variance;

    // ── 2. Wilder DMI / ADX ───────────────────────────────────────────────
    if (s.prevHLValid) {
        const double trueRange = std::max({high - low,
                                           std::abs(high - s.prevHigh),
                                           std::abs(low  - s.prevLow)});
        const double dmPlus  = (high - s.prevHigh > s.prevLow  - low  &&
                                 high - s.prevHigh > 0.0)
                               ? high - s.prevHigh : 0.0;
        const double dmMinus = (s.prevLow - low > high - s.prevHigh &&
                                 s.prevLow - low > 0.0)
                               ? s.prevLow - low : 0.0;

        ++s.adxObservations;

        // Seed Wilder smoothing for first adxPeriod observations
        if (s.adxObservations <= m_adxPeriod) {
            s.smoothTR      += trueRange;
            s.smoothDMPlus  += dmPlus;
            s.smoothDMMinus += dmMinus;

            if (s.adxObservations == m_adxPeriod) {
                // Compute initial DI values and first DX for ADX seeding
                const double diPlus  = s.smoothTR > 0.0
                                       ? (s.smoothDMPlus  / s.smoothTR) * 100.0 : 0.0;
                const double diMinus = s.smoothTR > 0.0
                                       ? (s.smoothDMMinus / s.smoothTR) * 100.0 : 0.0;
                const double diSum   = diPlus + diMinus;
                const double dx      = diSum > 0.0
                                       ? std::abs(diPlus - diMinus) / diSum * 100.0 : 0.0;
                s.adx = dx;
            }
        } else {
            // Wilder smoothing: S_new = S_old - (S_old / period) + current
            s.smoothTR      = s.smoothTR      - (s.smoothTR      / m_adxPeriod) + trueRange;
            s.smoothDMPlus  = s.smoothDMPlus  - (s.smoothDMPlus  / m_adxPeriod) + dmPlus;
            s.smoothDMMinus = s.smoothDMMinus - (s.smoothDMMinus / m_adxPeriod) + dmMinus;

            const double diPlus  = s.smoothTR > 0.0
                                   ? (s.smoothDMPlus  / s.smoothTR) * 100.0 : 0.0;
            const double diMinus = s.smoothTR > 0.0
                                   ? (s.smoothDMMinus / s.smoothTR) * 100.0 : 0.0;
            const double diSum   = diPlus + diMinus;
            const double dx      = diSum > 0.0
                                   ? std::abs(diPlus - diMinus) / diSum * 100.0 : 0.0;
            // ADX = Wilder-smoothed DX
            s.adx = s.adx - (s.adx / m_adxPeriod) + (dx / m_adxPeriod);
        }
    }
    s.prevHigh    = high;
    s.prevLow     = low;
    s.prevHLValid = true;
    m_adx         = s.adx;

    // ── 3. Classify regime ────────────────────────────────────────────────
    classifyRegime();
    s.regime = m_regime;
    return m_regime;
}

void RegimeDetector::classifyRegime() noexcept
{
    const bool highVariance = (m_variance > m_varThreshold);

    if (m_adx > m_adxTrendAbove && !highVariance) {
        m_regime = MarketRegime::TREND;
    } else if (m_adx < m_adxRangeBelow && !highVariance) {
        m_regime = MarketRegime::RANGE;
    } else {
        m_regime = MarketRegime::CHAOS;
    }
}

void RegimeDetector::restoreState(const State& s) noexcept
{
    m_state    = s;
    m_regime   = s.regime;
    m_adx      = s.adx;
    m_variance = s.variance;
}

// ── Static helpers ────────────────────────────────────────────────────────────

const char* RegimeDetector::regimeName(MarketRegime r) noexcept
{
    switch (r) {
        case MarketRegime::TREND: return "TREND";
        case MarketRegime::RANGE: return "RANGE";
        case MarketRegime::CHAOS: return "CHAOS";
    }
    return "UNKNOWN";
}

double RegimeDetector::affinity(const std::string& sid, MarketRegime r) noexcept
{
    // K_chaos: risk-off for all strategies
    if (r == MarketRegime::CHAOS) { return 0.0; }

    if (sid == "SMA_01") {
        return (r == MarketRegime::TREND) ? 1.0 : -0.5;  // RANGE = -0.5
    }
    if (sid == "MR_01") {
        return (r == MarketRegime::RANGE) ? 1.0 : -0.5;  // TREND = -0.5
    }
    return 0.0;
}
