#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Forward declaration — avoid circular include
class PortfolioAllocator;

// ─── Data-point structs ──────────────────────────────────────────────────────

struct TradeLogEntry {
    uint64_t  timestamp{0};
    char      side{' '};
    double    price{0.0};
    double    quantity{0.0};
    double    fee{0.0};
    double    realizedPnL{0.0};
    double    grossPnL{0.0};
    std::string strategy_id;
};

struct EquityPoint {
    uint64_t  timestamp{0};
    double    equity{0.0};
    double    drawdownPct{0.0};
    int       openPositions{0};
    double    var95{0.0};
};

struct SlippagePoint {
    uint64_t    timestamp{0};
    std::string symbol;
    double      expectedPrice{0.0};
    double      actualPrice{0.0};
    double      quantity{0.0};
    double      slippageBps{0.0};
};

struct RegimeMatrixPoint {
    uint64_t    timestamp{0};
    std::string symbol;
    std::string regime;
    double      adx{0.0};
    double      variance{0.0};
};

// ─── AnalyticsEngine ─────────────────────────────────────────────────────────

class AnalyticsEngine {
public:
    // Inject PortfolioAllocator for Phase 11 telemetry callbacks.
    // When set, every closed sell-leg trade fires allocator->onTradeClosed().
    void setPortfolioAllocator(PortfolioAllocator* allocator) noexcept;

    void recordTrade(uint64_t timestamp, char side,
                     double price, double quantity,
                     double fee, double realizedPnL = 0.0,
                     double grossPnL = 0.0,
                     const std::string& strategyId = "");

    void recordEquity(uint64_t timestamp, double equity,
                      double drawdownPct, int openPositions = 0,
                      double var95 = 0.0);
    void recordSlippage(uint64_t timestamp, const std::string& symbol,
                        double expectedPrice, double actualPrice,
                        double quantity);
    void recordRegimeMatrix(uint64_t timestamp, const std::string& symbol,
                            const std::string& regime,
                            double adx, double variance);

    bool finalize(const std::string& outputDir = "data/results") const;

    const std::vector<TradeLogEntry>& getTrades() const noexcept;
    const std::vector<EquityPoint>&   getEquity() const noexcept;
    const std::vector<SlippagePoint>& getSlippage() const noexcept;
    const std::vector<RegimeMatrixPoint>& getRegimeMatrix() const noexcept;
    double slippageEwmaBps() const noexcept;

private:
    std::vector<TradeLogEntry> m_trades;
    std::vector<EquityPoint>   m_equity;
    std::vector<SlippagePoint> m_slippage;
    std::vector<RegimeMatrixPoint> m_regimeMatrix;
    PortfolioAllocator*        m_allocator{nullptr};
    double m_slippageEwmaBps{0.0};
    double m_slippageAlpha{0.2};
};
