#include "ExecutionEngine.hpp"
#include "AnalyticsEngine.hpp"
#include "BrokerGateway.hpp"
#include "L2OrderBook.hpp"
#include "SmaCrossStrategy.hpp"
#include "TriggerOrderManager.hpp"
#include "MarketCandle.hpp"
#include <chrono>
#include <cmath>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <algorithm>

ExecutionEngine::ExecutionEngine(PortfolioManager& portfolio,
                                 RiskEngine&       riskEngine,
                                 std::string       symbol,
                                 double            feeRate,
                                 double            slippageBps,
                                 double            riskPct)
    : m_portfolio(portfolio)
    , m_riskEngine(riskEngine)
    , m_symbol(std::move(symbol))
    , m_feeRate(feeRate)
    , m_slippage(slippageBps / 10'000.0)
    , m_riskPct(riskPct)
{
    m_orderPool.resize(ORDER_POOL_SIZE);
    m_freeOrderNodes.reserve(ORDER_POOL_SIZE);
    for (auto& node : m_orderPool) {
        m_freeOrderNodes.push_back(&node);
    }
}

void ExecutionEngine::setAnalyticsEngine(AnalyticsEngine* analytics) noexcept
{
    m_analytics = analytics;
}

void ExecutionEngine::setStrategy(SmaCrossStrategy* strategy) noexcept
{
    m_strategy = strategy;
}

void ExecutionEngine::bindBrokerGateway(BrokerGateway* gateway) noexcept
{
    m_gateway = gateway;
    if (!m_gateway) { return; }
    m_gateway->addFillCallback([this](const BrokerFill& fill) {
        onBrokerFill(fill);
    });
}

ExecutionEngine::OrderNode* ExecutionEngine::acquireOrderNode() noexcept
{
    std::lock_guard<std::mutex> lock(m_poolMutex);
    if (m_freeOrderNodes.empty()) {
        return nullptr;
    }
    OrderNode* node = m_freeOrderNodes.back();
    m_freeOrderNodes.pop_back();
    node->inUse = true;
    return node;
}

void ExecutionEngine::releaseOrderNode(OrderNode* node) noexcept
{
    if (!node) { return; }
    std::lock_guard<std::mutex> lock(m_poolMutex);
    node->inUse = false;
    m_freeOrderNodes.push_back(node);
}

void ExecutionEngine::copyToFixed(std::array<char, 24>& dst,
                                  const std::string& src) noexcept
{
    dst.fill('\0');
    const std::size_t count = std::min(dst.size() - 1, src.size());
    std::memcpy(dst.data(), src.data(), count);
}

std::string ExecutionEngine::fromFixed(const std::array<char, 24>& src)
{
    return std::string(src.data());
}

void ExecutionEngine::queueOrderEvent(const OrderBusEvent& event) noexcept
{
    OrderNode* node = acquireOrderNode();
    if (!node) {
        m_droppedBusEvents.fetch_add(1);
        return;
    }
    node->event = event;
    if (!m_orderBus.push(node)) {
        m_droppedBusEvents.fetch_add(1);
        releaseOrderNode(node);
    }
}

void ExecutionEngine::drainOrderBus()
{
    if (!m_gateway || !m_gateway->isConnected()) {
        return;
    }

    OrderNode* node = nullptr;
    while (m_orderBus.pop(node)) {
        if (!node) { continue; }
        const auto dispatchNs =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        const double deltaMs = static_cast<double>(dispatchNs - node->event.signalNs) / 1'000'000.0;

        m_lastRouteLatencyMs.store(deltaMs);
        const double prevMax = m_maxRouteLatencyMs.load();
        if (deltaMs > prevMax) {
            m_maxRouteLatencyMs.store(deltaMs);
        }
        if (deltaMs > 5.0) {
            m_latencyBreaches.fetch_add(1);
            m_riskEngine.reportLatency(static_cast<uint32_t>(std::ceil(deltaMs)));
        }

        const std::string symbol = fromFixed(node->event.symbol);
        const std::string strategyId = fromFixed(node->event.strategyId);
        const BrokerFill fill = m_gateway->submitOrder(symbol,
                                                       node->event.isBuy,
                                                       node->event.quantity,
                                                       node->event.requestedPrice,
                                                       node->event.timestamp,
                                                       strategyId);
        {
            std::lock_guard<std::mutex> lock(m_fillMutex);
            m_orderStrategyMap[fill.orderId] = strategyId;
        }
        releaseOrderNode(node);
    }
}

bool ExecutionEngine::execute(Signal signal, double marketPrice, uint64_t timestamp,
                              const std::string& strategyId)
{
    if (signal == Signal::NONE) {
        return false;
    }

    ++m_signalCount;

    if (signal == Signal::BUY) {
        if (!m_riskEngine.canTrade()) {
            ++m_blockedCount;
            std::cout << "[RISK]  BUY blocked by RiskEngine"
                      << " | Price: "  << std::fixed << std::setprecision(2) << marketPrice
                      << " | Equity: " << m_portfolio.getTotalEquity()
                      << " | DD: "     << std::setprecision(4)
                      << (m_portfolio.getCurrentDrawdown() * 100.0) << "%"
                      << "\n";
            return false;
        }
        if (m_portfolio.hasPosition(m_symbol)) {
            return false;
        }

        // Buy-side slippage: fill above market.
        const double fillPrice = marketPrice * (1.0 + m_slippage);

        // ── Dynamic ATR-based position sizing ────────────────────────────
        const double      cash    = m_portfolio.getCashBalance();
        const std::size_t maxPos  = m_riskEngine.getMaxConcurrentPositions();
        const std::size_t openPos = m_portfolio.getOpenPositionCount();

        double units   = 0.0;
        double capital = 0.0;
        double fee     = 0.0;

        if (m_strategy != nullptr && m_strategy->isATRValid()
            && m_strategy->getATR() > 0.0)
        {
            // ATR parity sizing: Units = (E_total * riskPct) / ATR(t)
            const double equity = m_portfolio.getTotalEquity();
            const double atr    = m_strategy->getATR();
            units   = (equity * m_riskPct) / atr;
            capital = units * fillPrice;
            fee     = capital * m_feeRate;
            // Safety cap: never commit more than available cash
            if (capital + fee > cash) {
                const double scaleFactor = cash / (capital + fee);
                units   *= scaleFactor;
                capital *= scaleFactor;
                fee      = capital * m_feeRate;
            }
            std::cout << "[SIZE]  ATR=" << std::fixed << std::setprecision(4) << atr
                      << " | Units=" << std::setprecision(4) << units
                      << " | Capital=$" << std::setprecision(2) << capital
                      << "\n";
        } else {
            // Fallback: equal-slice capital allocation
            const std::size_t remaining = (maxPos > openPos) ? (maxPos - openPos) : 1;
            capital = (maxPos == 0) ? cash : cash / static_cast<double>(remaining);
            fee     = capital * m_feeRate;
        }

        if (m_gateway && m_gateway->isConnected()) {
            OrderBusEvent evt;
            evt.orderLocalId = m_nextLocalOrderId.fetch_add(1);
            copyToFixed(evt.symbol, m_symbol);
            copyToFixed(evt.strategyId, strategyId);
            evt.isBuy = true;
            evt.quantity = (units > 0.0) ? units : ((capital - fee) / fillPrice);
            evt.requestedPrice = marketPrice;
            evt.timestamp = timestamp;
            evt.signalNs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());

            queueOrderEvent(evt);
            drainOrderBus();
            return true;
        } else {
            if (units > 0.0) {
                m_portfolio.openLong(m_symbol, fillPrice, timestamp, fee,
                                     capital, units, strategyId);
            } else {
                m_portfolio.openLong(m_symbol, fillPrice, timestamp, fee,
                                     capital, 0.0, strategyId);
            }
            ++m_filledCount;

            if (m_analytics) {
                m_analytics->recordTrade(timestamp, 'B', fillPrice,
                                         m_portfolio.getPositionQuantity(m_symbol),
                                         fee, 0.0, 0.0, strategyId);
                m_analytics->recordSlippage(timestamp, m_symbol, marketPrice,
                                            fillPrice,
                                            m_portfolio.getPositionQuantity(m_symbol));
            }

            std::cout << "[TRADE] BUY"
                      << " | Symbol: "   << m_symbol
                      << " | Strategy: " << (strategyId.empty() ? "N/A" : strategyId)
                      << " | Fill: $"    << std::fixed << std::setprecision(2) << fillPrice
                      << " | Fee: $"     << std::setprecision(2) << fee
                      << " | Equity: "   << m_portfolio.getTotalEquity()
                      << " | DD: "       << std::setprecision(4)
                      << (m_portfolio.getCurrentDrawdown() * 100.0) << "%"
                      << "\n";
            return true;
        }
    }

    if (signal == Signal::SELL) {
        if (!m_portfolio.hasPosition(m_symbol)) {
            return false;
        }

        // Sell-side slippage: fill below market.
        const double fillPrice = marketPrice * (1.0 - m_slippage);
        const double qty       = m_portfolio.getPositionQuantity(m_symbol);
        const double proceeds  = qty * fillPrice;
        const double fee       = proceeds * m_feeRate;

        if (m_gateway && m_gateway->isConnected()) {
            OrderBusEvent evt;
            evt.orderLocalId = m_nextLocalOrderId.fetch_add(1);
            copyToFixed(evt.symbol, m_symbol);
            copyToFixed(evt.strategyId, strategyId);
            evt.isBuy = false;
            evt.quantity = qty;
            evt.requestedPrice = marketPrice;
            evt.timestamp = timestamp;
            evt.signalNs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
            queueOrderEvent(evt);
            drainOrderBus();
            return true;
        } else {
            m_portfolio.closePosition(m_symbol, fillPrice, timestamp, fee, strategyId);
            ++m_filledCount;

            // Retrieve accurate PnL from the just-recorded TradeRecord.
            double accuratePnL   = 0.0;
            double accurateGross = 0.0;
            const auto& log = m_portfolio.getTradeLog();
            if (!log.empty()) {
                accuratePnL  = log.back().realizedPnL;
                accurateGross = log.back().grossPnL;
            }

            if (m_analytics) {
                m_analytics->recordTrade(timestamp, 'S', fillPrice, qty, fee,
                                         accuratePnL, accurateGross, strategyId);
                m_analytics->recordSlippage(timestamp, m_symbol, marketPrice,
                                            fillPrice, qty);
            }

            std::cout << "[TRADE] SELL"
                      << " | Symbol: "   << m_symbol
                      << " | Strategy: " << (strategyId.empty() ? "N/A" : strategyId)
                      << " | Fill: $"    << std::fixed << std::setprecision(2) << fillPrice
                      << " | Fee: $"     << std::setprecision(2) << fee
                      << " | PnL: $"     << std::setprecision(2) << accuratePnL
                      << " | Equity: "   << m_portfolio.getTotalEquity()
                      << " | DD: "       << std::setprecision(4)
                      << (m_portfolio.getCurrentDrawdown() * 100.0) << "%"
                      << "\n";
            return true;
        }
    }

    return false;
}

int ExecutionEngine::getSignalCount()  const noexcept { return m_signalCount; }
int ExecutionEngine::getFilledCount()  const noexcept { return m_filledCount; }
int ExecutionEngine::getBlockedCount() const noexcept { return m_blockedCount; }
double ExecutionEngine::getLastRouteLatencyMs() const noexcept { return m_lastRouteLatencyMs.load(); }
double ExecutionEngine::getMaxRouteLatencyMs() const noexcept { return m_maxRouteLatencyMs.load(); }
uint64_t ExecutionEngine::getLatencyBreachCount() const noexcept { return m_latencyBreaches.load(); }
uint64_t ExecutionEngine::getDroppedBusEvents() const noexcept { return m_droppedBusEvents.load(); }
uint64_t ExecutionEngine::lastBrokerOrderId() const noexcept { return m_lastBrokerOrderId.load(); }

double ExecutionEngine::pendingBrokerQuantity(uint64_t orderId) const noexcept
{
    std::lock_guard<std::mutex> lock(m_fillMutex);
    auto it = m_pendingBrokerQty.find(orderId);
    return (it == m_pendingBrokerQty.end()) ? 0.0 : it->second;
}

void ExecutionEngine::onBrokerFill(const BrokerFill& fill)
{
    if (fill.symbol != m_symbol) {
        return;
    }
    m_lastBrokerOrderId.store(fill.orderId);

    if (!fill.success) {
        m_riskEngine.reportApiError();
        return;
    }

    const double executedQty = (fill.filledQuantity > 0.0)
                               ? fill.filledQuantity
                               : fill.quantity;
    if (executedQty <= 0.0) {
        return;
    }

    std::string strategyId;
    {
        std::lock_guard<std::mutex> lock(m_fillMutex);
        auto it = m_orderStrategyMap.find(fill.orderId);
        if (it != m_orderStrategyMap.end()) {
            strategyId = it->second;
        }
        if (!fill.strategyId.empty()) {
            strategyId = fill.strategyId;
        }
    }

    if (fill.isBuy) {
        const double capital = (fill.fillPrice * executedQty) + fill.feePaid;
        if (!m_portfolio.hasPosition(fill.symbol)) {
            m_portfolio.openLong(fill.symbol,
                                 fill.fillPrice,
                                 fill.fillTimestamp,
                                 fill.feePaid,
                                 capital,
                                 executedQty,
                                 strategyId);
        } else {
            m_portfolio.addToLong(fill.symbol,
                                  fill.fillPrice,
                                  executedQty,
                                  fill.feePaid,
                                  capital,
                                  strategyId);
        }
        if (m_analytics) {
            m_analytics->recordTrade(fill.fillTimestamp, 'B', fill.fillPrice,
                                     executedQty, fill.feePaid, 0.0, 0.0, strategyId);
            m_analytics->recordSlippage(fill.fillTimestamp, fill.symbol,
                                        fill.requestedPrice, fill.fillPrice,
                                        executedQty);
        }
    } else {
        const double qtyHeld = m_portfolio.getPositionQuantity(fill.symbol);
        const double closeQty = std::min(qtyHeld, executedQty);
        m_portfolio.closePosition(fill.symbol,
                                  fill.fillPrice,
                                  fill.fillTimestamp,
                                  fill.feePaid,
                                  strategyId,
                                  closeQty);
        double accuratePnL = 0.0;
        double accurateGross = 0.0;
        const auto& log = m_portfolio.getTradeLog();
        if (!log.empty()) {
            accuratePnL   = log.back().realizedPnL;
            accurateGross = log.back().grossPnL;
        }
        if (m_analytics) {
            m_analytics->recordTrade(fill.fillTimestamp, 'S', fill.fillPrice,
                                     closeQty, fill.feePaid,
                                     accuratePnL, accurateGross, strategyId);
            m_analytics->recordSlippage(fill.fillTimestamp, fill.symbol,
                                        fill.requestedPrice, fill.fillPrice,
                                        closeQty);
        }
    }

    m_riskEngine.syncPosition(fill.symbol, m_portfolio.getPositionQuantity(fill.symbol));
    ++m_filledCount;

    {
        std::lock_guard<std::mutex> lock(m_fillMutex);
        if (fill.partialFill && fill.remainingQuantity > 0.0) {
            m_pendingBrokerQty[fill.orderId] = fill.remainingQuantity;
        } else {
            m_pendingBrokerQty.erase(fill.orderId);
            m_orderStrategyMap.erase(fill.orderId);
        }
    }
}

// ── Phase 9: Pending Order Lifecycle ─────────────────────────────────────────

uint64_t ExecutionEngine::placePendingOrder(const OrderRecord& order) noexcept
{
    return m_portfolio.placePendingOrder(order);
}

void ExecutionEngine::bindTriggerOrderManager(TriggerOrderManager* manager) noexcept
{
    m_triggerOrders = manager;
}

uint64_t ExecutionEngine::placeStopLossTrigger(double stopPrice,
                                               double quantity,
                                               const std::string& strategyId) noexcept
{
    if (!m_triggerOrders) {
        return 0;
    }
    const double qty = (quantity > 0.0) ? quantity : m_portfolio.getPositionQuantity(m_symbol);
    return m_triggerOrders->placeStopLoss(m_symbol,
                                          /*isBuy=*/false,
                                          stopPrice,
                                          qty,
                                          strategyId);
}

uint64_t ExecutionEngine::placeOcoTrigger(double stopPrice,
                                          double takeProfitPrice,
                                          double quantity,
                                          const std::string& strategyId) noexcept
{
    if (!m_triggerOrders) {
        return 0;
    }
    const double qty = (quantity > 0.0) ? quantity : m_portfolio.getPositionQuantity(m_symbol);
    return m_triggerOrders->placeOco(m_symbol,
                                     /*isBuy=*/false,
                                     stopPrice,
                                     takeProfitPrice,
                                     qty,
                                     strategyId);
}

int ExecutionEngine::processTriggerOrders(const L2OrderBook& orderBook,
                                          uint64_t timestamp)
{
    if (!m_triggerOrders) {
        return 0;
    }

    const auto bbo = orderBook.bbo();
    if (!bbo.valid) {
        return 0;
    }

    std::vector<TriggerFillEvent> triggers;
    triggers.reserve(16);
    const std::size_t triggeredCount =
        m_triggerOrders->evaluate(m_symbol, bbo, timestamp, triggers);

    int executed = 0;
    for (const auto& ev : triggers) {
        const Signal signal = ev.isBuy ? Signal::BUY : Signal::SELL;
        const std::string sid = ev.strategyId.empty() ? "TRIGGER" : ev.strategyId;
        if (execute(signal, ev.executionPrice, ev.timestamp, sid)) {
            ++executed;
        }
    }
    return static_cast<int>(std::min<std::size_t>(static_cast<std::size_t>(executed),
                                                  triggeredCount));
}

int ExecutionEngine::processPendingOrders(const MarketCandle& candle)
{
    std::vector<OrderFillResult> fills =
        m_portfolio.evaluatePendingOrders(candle.symbol,
                                          candle.high,
                                          candle.low,
                                          candle.close,
                                          candle.epochTimestamp);
    int filledCount = 0;
    for (const auto& res : fills) {
        if (!res.filled) { continue; }

        const double fillPrice = res.fillPrice;

        if (res.isBuy) {
            if (!m_riskEngine.canTrade()) {
                ++m_blockedCount;
                std::cout << "[RISK]  Pending BUY (id=" << res.orderId
                          << ") blocked by RiskEngine at $"
                          << std::fixed << std::setprecision(2) << fillPrice << "\n";
                continue;
            }
            if (m_portfolio.hasPosition(res.symbol)) {
                continue;
            }

            const double slippedPrice = fillPrice * (1.0 + m_slippage);
            const double cash   = m_portfolio.getCashBalance();
            double capital      = (res.capitalToCommit > 0.0 && res.capitalToCommit <= cash)
                                  ? res.capitalToCommit
                                  : cash;
            double units        = res.quantity;
            double fee          = (units > 0.0)
                                  ? units * slippedPrice * m_feeRate
                                  : capital * m_feeRate;

            if (units > 0.0) {
                capital = units * slippedPrice + fee;
                if (capital > cash) {
                    const double scale = cash / capital;
                    units   *= scale;
                    capital *= scale;
                    fee      = capital * m_feeRate;
                }
                m_portfolio.openLong(res.symbol, slippedPrice,
                                     res.fillTimestamp, fee, capital, units);
            } else {
                m_portfolio.openLong(res.symbol, slippedPrice,
                                     res.fillTimestamp, fee, capital);
            }
            ++m_filledCount;
            ++filledCount;

            if (m_analytics) {
                m_analytics->recordTrade(res.fillTimestamp, 'B', slippedPrice,
                                         m_portfolio.getPositionQuantity(res.symbol),
                                         fee, 0.0);
            }
            std::cout << "[PENDING] BUY FILL id=" << res.orderId
                      << " sym=" << res.symbol
                      << " price=$" << std::fixed << std::setprecision(2) << slippedPrice
                      << "\n";
        } else {
            if (!m_portfolio.hasPosition(res.symbol)) { continue; }

            const double slippedPrice = fillPrice * (1.0 - m_slippage);
            const double qty          = m_portfolio.getPositionQuantity(res.symbol);
            const double proceeds     = qty * slippedPrice;
            const double fee          = proceeds * m_feeRate;

            m_portfolio.closePosition(res.symbol, slippedPrice, res.fillTimestamp, fee);
            ++m_filledCount;
            ++filledCount;

            double accuratePnL   = 0.0;
            double accurateGross = 0.0;
            const auto& log = m_portfolio.getTradeLog();
            if (!log.empty()) {
                accuratePnL  = log.back().realizedPnL;
                accurateGross = log.back().grossPnL;
            }
            if (m_analytics) {
                m_analytics->recordTrade(res.fillTimestamp, 'S', slippedPrice,
                                         qty, fee, accuratePnL, accurateGross);
            }
            std::cout << "[PENDING] SELL FILL id=" << res.orderId
                      << " sym=" << res.symbol
                      << " price=$" << std::fixed << std::setprecision(2) << slippedPrice
                      << " PnL=$" << std::setprecision(2) << accuratePnL << "\n";
        }
    }
    return filledCount;
}
