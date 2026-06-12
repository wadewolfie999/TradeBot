#pragma once
#include "IStrategy.hpp"
#include "PortfolioAllocator.hpp"
#include "RegimeDetector.hpp"
#include "RiskEngine.hpp"
#include "MarketCandle.hpp"
#include "ExecutionEngine.hpp"
#include "PortfolioManager.hpp"
#include <cstdint>
#include <vector>

class AnalyticsEngine;
class StateSerializer;

class EventLoop {
public:
    // Single-strategy constructor (backward-compatible).
    EventLoop(IStrategy&        strategy,
              RiskEngine&        riskEngine,
              ExecutionEngine&   executionEngine,
              PortfolioManager&  portfolio);

    // Phase 10 / 11: multi-strategy constructor using a PortfolioAllocator.
    EventLoop(std::vector<IStrategy*> strategies,
              PortfolioAllocator&     allocator,
              RiskEngine&             riskEngine,
              ExecutionEngine&        executionEngine,
              PortfolioManager&       portfolio);

    void setAnalyticsEngine(AnalyticsEngine* analytics) noexcept;

    // Phase 9 serializer (legacy save, no regime state).
    void setStateSerializer(StateSerializer* serializer,
                            uint64_t checkpointIntervalSecs = 86400) noexcept;

    // Phase 11: inject RegimeDetector so EventLoop drives regime updates and
    // uses the full Phase 11 saveSnapshot overload.
    void setRegimeDetector(RegimeDetector* rd) noexcept;

    void processCandle(const MarketCandle& candle);

    int getTotalSignals()    const noexcept;
    int getRiskBlockedBuys() const noexcept;

private:
    void dispatchSignal(Signal signal, double price, uint64_t timestamp,
                        const std::string& strategyId);

    IStrategy*              m_singleStrategy{nullptr};
    std::vector<IStrategy*> m_strategies;
    PortfolioAllocator*     m_allocator{nullptr};
    RegimeDetector*         m_regimeDetector{nullptr};

    RiskEngine&        m_riskEngine;
    ExecutionEngine&   m_executionEngine;
    PortfolioManager&  m_portfolio;
    AnalyticsEngine*   m_analytics{nullptr};

    int m_totalSignals{0};
    int m_riskBlockedBuys{0};

    StateSerializer* m_serializer{nullptr};
    uint64_t         m_checkpointIntervalSec{0};
    uint64_t         m_lastCheckpointTs{0};
};
