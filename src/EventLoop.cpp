#include "EventLoop.hpp"
#include "AnalyticsEngine.hpp"
#include "StateSerializer.hpp"
#include <iostream>
#include <iomanip>

// ── Constructors ──────────────────────────────────────────────────────────────

EventLoop::EventLoop(IStrategy&        strategy,
                     RiskEngine&        riskEngine,
                     ExecutionEngine&   executionEngine,
                     PortfolioManager&  portfolio)
    : m_singleStrategy(&strategy)
    , m_riskEngine(riskEngine)
    , m_executionEngine(executionEngine)
    , m_portfolio(portfolio)
{}

EventLoop::EventLoop(std::vector<IStrategy*> strategies,
                     PortfolioAllocator&     allocator,
                     RiskEngine&             riskEngine,
                     ExecutionEngine&        executionEngine,
                     PortfolioManager&       portfolio)
    : m_strategies(std::move(strategies))
    , m_allocator(&allocator)
    , m_riskEngine(riskEngine)
    , m_executionEngine(executionEngine)
    , m_portfolio(portfolio)
{}

void EventLoop::setAnalyticsEngine(AnalyticsEngine* analytics) noexcept
{
    m_analytics = analytics;
}

void EventLoop::setStateSerializer(StateSerializer* serializer,
                                    uint64_t checkpointIntervalSecs) noexcept
{
    m_serializer            = serializer;
    m_checkpointIntervalSec = checkpointIntervalSecs;
}

void EventLoop::setRegimeDetector(RegimeDetector* rd) noexcept
{
    m_regimeDetector = rd;
}

// ── Private helpers ───────────────────────────────────────────────────────────

void EventLoop::dispatchSignal(Signal signal, double price, uint64_t timestamp,
                               const std::string& strategyId)
{
    if (signal != Signal::NONE) { ++m_totalSignals; }

    if (m_riskEngine.canTrade()) {
        m_executionEngine.execute(signal, price, timestamp, strategyId);
    } else {
        if (signal == Signal::SELL) {
            m_executionEngine.execute(signal, price, timestamp, strategyId);
        } else if (signal == Signal::BUY) {
            ++m_riskBlockedBuys;
            std::cout << "[RISK]  BUY blocked by RiskEngine"
                      << " | Price: "  << std::fixed << std::setprecision(2) << price
                      << " | Equity: " << m_portfolio.getTotalEquity()
                      << " | DD: "     << std::setprecision(4)
                      << (m_portfolio.getCurrentDrawdown() * 100.0) << "%"
                      << "\n";
        }
    }
}

// ── Core candle processing ────────────────────────────────────────────────────

void EventLoop::processCandle(const MarketCandle& candle)
{
    std::cout << "[Engine] Parsed: "
              << candle.epochTimestamp
              << " | Sym: " << candle.symbol
              << " | O: " << candle.open
              << " H: "   << candle.high
              << " L: "   << candle.low
              << " C: "   << candle.close
              << " V: "   << candle.volume
              << "\n";

    m_executionEngine.processPendingOrders(candle);

    // ── Phase 11: update regime before ensembling ─────────────────────────
    if (m_regimeDetector != nullptr) {
        m_regimeDetector->update(candle.high, candle.low, candle.close);
    }

    // ── Signal generation ─────────────────────────────────────────────────
    if (m_allocator != nullptr && !m_strategies.empty()) {
        std::vector<AlphaSignal> alphaSignals;
        alphaSignals.reserve(m_strategies.size());

        for (IStrategy* strat : m_strategies) {
            AlphaSignal alpha = strat->generateSignal(candle);
            alphaSignals.push_back(alpha);
        }

        const AllocationResult allocation = m_allocator->ensemble(alphaSignals);

        const std::string attrId = !allocation.dominantStrategyId.empty()
                                   ? allocation.dominantStrategyId
                                   : "ENSEMBLE";

        dispatchSignal(allocation.action, candle.close, candle.epochTimestamp, attrId);
    } else if (m_singleStrategy != nullptr) {
        Signal signal = m_singleStrategy->onCandle(candle);
        if (signal != Signal::NONE) { ++m_totalSignals; }

        if (m_riskEngine.canTrade()) {
            m_executionEngine.execute(signal, candle.close, candle.epochTimestamp);
        } else {
            if (signal == Signal::SELL) {
                m_executionEngine.execute(signal, candle.close, candle.epochTimestamp);
            } else if (signal == Signal::BUY) {
                ++m_riskBlockedBuys;
                std::cout << "[RISK]  BUY blocked by RiskEngine"
                          << " | Price: "  << std::fixed << std::setprecision(2) << candle.close
                          << " | Equity: " << m_portfolio.getTotalEquity()
                          << " | DD: "     << std::setprecision(4)
                          << (m_portfolio.getCurrentDrawdown() * 100.0) << "%"
                          << "\n";
            }
        }
    }

    // ── Post-candle portfolio & risk updates ──────────────────────────────
    m_portfolio.updatePnL(candle.symbol, candle.close);
    m_riskEngine.setTotalDrawdown(m_portfolio.getCurrentDrawdown());

    {
        const double posVal = m_portfolio.getPositionQuantity(candle.symbol)
                            * candle.close;
        m_riskEngine.pushAssetReturn(candle.symbol, candle.close, posVal);
    }

    m_riskEngine.pushEquityReturn(m_portfolio.getTotalEquity());

    if (m_analytics) {
        m_analytics->recordEquity(candle.epochTimestamp,
                                  m_portfolio.getTotalEquity(),
                                  m_portfolio.getCurrentDrawdown() * 100.0,
                                  static_cast<int>(m_portfolio.getOpenPositionCount()),
                                  m_riskEngine.getVaR95());
    }

    // ── Periodic checkpoint ───────────────────────────────────────────────
    if (m_serializer && m_checkpointIntervalSec > 0) {
        if (m_lastCheckpointTs == 0) {
            m_lastCheckpointTs = candle.epochTimestamp;
        } else if (candle.epochTimestamp - m_lastCheckpointTs
                   >= m_checkpointIntervalSec) {
            if (m_regimeDetector != nullptr && m_allocator != nullptr) {
                m_serializer->saveSnapshot(m_portfolio, m_riskEngine,
                                           *m_regimeDetector, *m_allocator,
                                           candle.epochTimestamp);
            } else {
                m_serializer->saveSnapshot(m_portfolio, m_riskEngine,
                                           candle.epochTimestamp);
            }
            m_lastCheckpointTs = candle.epochTimestamp;
        }
    }
}

int EventLoop::getTotalSignals()    const noexcept { return m_totalSignals; }
int EventLoop::getRiskBlockedBuys() const noexcept { return m_riskBlockedBuys; }
