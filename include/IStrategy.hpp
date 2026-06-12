#pragma once
#include "MarketCandle.hpp"
#include <string>

// ── Phase 10: Alpha Signal Interface ─────────────────────────────────────────
//
// Strategies no longer emit binary BUY/SELL decisions. Instead each strategy
// returns a continuous conviction score S ∈ [-1.0, 1.0] tagged with its own
// identifier. The PortfolioAllocator ensembles multiple AlphaSignal objects
// into an aggregate position target before routing to the ExecutionEngine.
//
// Convention:
//   +1.0  strong long conviction  (max bullish)
//    0.0  neutral / no view
//   -1.0  strong short conviction (max bearish)

struct AlphaSignal {
    std::string symbol;       // instrument this signal applies to
    std::string strategy_id;  // unique strategy identifier, e.g. "SMA_01"
    double      conviction;   // normalised score S ∈ [-1.0, 1.0]
};

// Legacy binary signal kept for backward-compatible components that have not
// yet been migrated to the AlphaSignal path.
enum class Signal { NONE, BUY, SELL };

// Convenience: derive a legacy Signal from an AlphaSignal conviction score.
// Threshold ±0.1 avoids thrashing around zero.
inline Signal alphaToSignal(const AlphaSignal& a) noexcept
{
    if (a.conviction >=  0.1) { return Signal::BUY;  }
    if (a.conviction <= -0.1) { return Signal::SELL; }
    return Signal::NONE;
}

class IStrategy {
public:
    virtual ~IStrategy() = default;

    // Called once per incoming candle.
    // Returns an AlphaSignal with conviction S ∈ [-1.0, 1.0].
    // conviction == 0.0 means no view this bar.
    virtual AlphaSignal generateSignal(const MarketCandle& candle) = 0;

    // Legacy shim: calls generateSignal and converts via alphaToSignal.
    virtual Signal onCandle(const MarketCandle& candle) {
        return alphaToSignal(generateSignal(candle));
    }
};
