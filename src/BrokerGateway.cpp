// BrokerGateway.cpp -- Phase 12 Workstream 7.3
//
// Simulation-layer broker gateway.  All order routing, fill confirmations,
// and reconciliation are handled locally.  In LIVE mode these code paths
// would be replaced by real HTTP/WSS calls, but the interface contract and
// accounting delegation to PortfolioManager remain identical.
#include "BrokerGateway.hpp"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <thread>

// Simulated slippage: 5 basis points one-way (matches ExecutionEngine default).
static constexpr double SIMULATED_SLIPPAGE_BPS = 5.0;

// -- Construction ------------------------------------------------------------

BrokerGateway::BrokerGateway(const SystemConfig& cfg,
                               PortfolioManager&   portfolio) noexcept
    : m_cfg(cfg)
    , m_portfolio(portfolio)
{}

// -- Connection lifecycle ----------------------------------------------------

void BrokerGateway::connect()
{
    if (m_cfg.isBacktest()) { return; }  // BACKTEST: not used

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_connected.load()) { return; }
    const bool hadConnectedBefore = (m_connectLifecycleCount.load() > 0);

    std::cout << "[BrokerGateway] Connecting to REST endpoint: "
              << m_cfg.restEndpoint << "\n";
    std::cout << "[BrokerGateway] Opening WSS fill stream:     "
              << m_cfg.wssEndpoint  << "\n";

    m_connected.store(true);
    m_connectLifecycleCount.fetch_add(1);
    if (hadConnectedBefore) {
        m_reconnectLifecycleCount.fetch_add(1);
    }
    std::cout << "[BrokerGateway] Connected ("
              << modeName(m_cfg.mode) << " mode).\n";
}

void BrokerGateway::disconnect() noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_connected.load()) { return; }
    m_connected.store(false);
    m_disconnectLifecycleCount.fetch_add(1);
    std::cout << "[BrokerGateway] Disconnected.\n";
}

bool BrokerGateway::isConnected() const noexcept
{
    return m_connected.load();
}

// -- Order management --------------------------------------------------------

BrokerFill BrokerGateway::submitOrder(const std::string& symbol,
                                       bool               isBuy,
                                       double             quantity,
                                       double             requestedPrice,
                                       uint64_t           timestamp,
                                       const std::string& strategyId)
{
    if (!m_connected.load() && !m_cfg.isBacktest()) {
        connect();
    }

    m_totalOrdersSubmitted.fetch_add(1);

    BrokerFill fill;
    fill.orderId         = m_nextOrderId.fetch_add(1);
    fill.symbol          = symbol;
    fill.isBuy           = isBuy;
    fill.requestedPrice  = requestedPrice;
    fill.fillTimestamp   = timestamp;
    fill.strategyId      = strategyId;

    // Check for injected error (used by circuit-breaker tests).
    bool faultDrop = false;
    bool applyLatencySpike = false;
    uint32_t latencyMs = 0;
    bool faultDisconnect = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        ++m_faultSequence;
        const uint64_t seq = m_faultSequence;
        if (m_faultCfg.enabled) {
            if (m_faultCfg.dropEveryN > 0 && (seq % m_faultCfg.dropEveryN) == 0) {
                faultDrop = true;
                m_faultDropCount.fetch_add(1);
            }
            if (m_faultCfg.latencySpikeEveryN > 0 && m_faultCfg.latencySpikeMs > 0
                && (seq % m_faultCfg.latencySpikeEveryN) == 0) {
                applyLatencySpike = true;
                latencyMs = m_faultCfg.latencySpikeMs;
                m_faultLatencyCount.fetch_add(1);
            }
            if (m_faultCfg.disconnectEveryN > 0 && (seq % m_faultCfg.disconnectEveryN) == 0) {
                faultDisconnect = true;
                if (m_connected.exchange(false)) {
                    m_disconnectLifecycleCount.fetch_add(1);
                }
                m_faultDisconnectCount.fetch_add(1);
            }
        }

        if (m_injectError) {
            fill.success      = false;
            fill.errorMessage = m_injectedErrorMsg;
            m_injectError     = false;
            m_totalApiErrors.fetch_add(1);
            std::cout << "[BrokerGateway] Order #" << fill.orderId
                      << " REJECTED: " << fill.errorMessage << "\n";
            return fill;
        }
    }
    if (faultDisconnect && !m_cfg.isBacktest()) {
        connect();
    }
    if (applyLatencySpike && latencyMs > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(latencyMs));
    }
    if (faultDrop) {
        fill.success = false;
        fill.errorMessage = "FAULT_INJECTOR_DROP_SIMULATED_TCP_PACKET_LOSS";
        m_totalApiErrors.fetch_add(1);
        std::cout << "[BrokerGateway] Order #" << fill.orderId
                  << " DROPPED by fault injector\n";
        return fill;
    }

    // Simulate fill: apply slippage, compute fee.
    const double fillPrice = applySlippage(requestedPrice, isBuy);
    const double feeRate   = 0.001;  // 0.1% (matches ExecutionEngine)
    double filledQuantity  = quantity;
    double remainingQty    = 0.0;
    bool   partial         = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_injectPartialFill &&
            m_partialFillRatio > 0.0 &&
            m_partialFillRatio < 1.0 &&
            quantity > 0.0) {
            partial        = true;
            filledQuantity = quantity * m_partialFillRatio;
            remainingQty   = std::max(0.0, quantity - filledQuantity);
            m_injectPartialFill = false;
            if (remainingQty > 0.0) {
                m_pendingRemainingQty[fill.orderId] = remainingQty;
            }
        }
    }

    const double notional  = fillPrice * filledQuantity;
    const double fee       = notional * feeRate;

    fill.fillPrice = fillPrice;
    fill.quantity  = quantity;
    fill.filledQuantity = filledQuantity;
    fill.remainingQuantity = remainingQty;
    fill.partialFill = partial;
    fill.feePaid   = fee;
    fill.success   = true;

    // Log the fill.
    std::cout << "[BrokerGateway] Order #" << fill.orderId
              << "  " << symbol
              << "  " << (isBuy ? "BUY " : "SELL")
              << "  qty=" << std::fixed << std::setprecision(6) << quantity
              << "  req=" << requestedPrice
              << "  fill=" << fillPrice
              << "  filled=" << filledQuantity
              << "  remain=" << remainingQty
              << "  fee=$" << std::setprecision(4) << fee
              << "  [" << modeName(m_cfg.mode) << "]\n";

    m_totalFillsReceived.fetch_add(1);

    // Notify the fill callback (ExecutionEngine updates live PnL).
    if (m_fillCallback) { m_fillCallback(fill); }
    for (const auto& cb : m_fillCallbacks) {
        if (cb) { cb(fill); }
    }

    return fill;
}

bool BrokerGateway::cancelOrder(uint64_t orderId)
{
    std::cout << "[BrokerGateway] Cancel request for order #" << orderId << "\n";
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingRemainingQty.erase(orderId);
    }
    // In PAPER mode: cancel is always successful (no real pending queue here;
    // PortfolioManager's pending order queue handles the actual removal).
    return true;
}

// -- Portfolio reconciliation ------------------------------------------------

void BrokerGateway::reconcile(uint64_t nowTimestamp)
{
    m_reconciliationCount.fetch_add(1);

    const double localCash   = m_portfolio.getCashBalance();
    const double localEquity = m_portfolio.getTotalEquity();

    BrokerAccount snap;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        snap = m_lastSnapshot;
    }

    std::cout << "[BrokerGateway] Reconciliation #" << m_reconciliationCount.load()
              << "  ts=" << nowTimestamp << "\n"
              << "    Local   cash=$" << std::fixed << std::setprecision(2) << localCash
              << "  equity=$" << localEquity << "\n";

    if (snap.snapshotTimestamp > 0) {
        const double cashDelta   = localCash   - snap.cashBalance;
        const double equityDelta = localEquity - snap.totalEquity;
        std::cout << "    Broker  cash=$" << snap.cashBalance
                  << "  equity=$" << snap.totalEquity << "\n"
                  << "    Delta   cash=$" << cashDelta
                  << "  equity=$" << equityDelta;
        if (std::abs(cashDelta) > 1.0 || std::abs(equityDelta) > 1.0) {
            std::cout << "  [WARN: reconciliation discrepancy detected]";
        }
        std::cout << "\n";
    } else {
        std::cout << "    (no broker snapshot yet -- first reconciliation)\n";
    }
}

void BrokerGateway::injectBrokerSnapshot(const BrokerAccount& snap) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_lastSnapshot = snap;
}

BrokerAccount BrokerGateway::lastBrokerSnapshot() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastSnapshot;
}

// -- Fill callback -----------------------------------------------------------

void BrokerGateway::setFillCallback(FillCallback cb) noexcept
{
    m_fillCallback = std::move(cb);
}

void BrokerGateway::addFillCallback(FillCallback cb) noexcept
{
    if (!cb) { return; }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_fillCallbacks.push_back(std::move(cb));
}

// -- Simulation harness ------------------------------------------------------

void BrokerGateway::simulateFillConfirmation(const BrokerFill& fill)
{
    m_totalFillsReceived.fetch_add(1);
    std::cout << "[BrokerGateway] Simulated fill confirmation: order #"
              << fill.orderId << "  " << fill.symbol
              << (fill.isBuy ? " BUY" : " SELL")
              << "  price=" << fill.fillPrice
              << "  qty="   << fill.quantity << "\n";
    if (m_fillCallback) { m_fillCallback(fill); }
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (fill.remainingQuantity > 0.0) {
            m_pendingRemainingQty[fill.orderId] = fill.remainingQuantity;
        } else {
            m_pendingRemainingQty.erase(fill.orderId);
        }
    }
    for (const auto& cb : m_fillCallbacks) {
        if (cb) { cb(fill); }
    }
}

void BrokerGateway::injectNextOrderError(const std::string& errorMessage) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_injectError     = true;
    m_injectedErrorMsg = errorMessage;
}

void BrokerGateway::injectNextPartialFill(double fillRatio) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (fillRatio <= 0.0 || fillRatio >= 1.0) {
        m_injectPartialFill = false;
        m_partialFillRatio  = 1.0;
        return;
    }
    m_injectPartialFill = true;
    m_partialFillRatio  = fillRatio;
}

void BrokerGateway::setFaultInjectorConfig(const FaultInjectorConfig& cfg) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_faultCfg = cfg;
}

BrokerGateway::FaultInjectorConfig BrokerGateway::faultInjectorConfig() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_faultCfg;
}

// -- Metrics -----------------------------------------------------------------

uint64_t BrokerGateway::totalOrdersSubmitted() const noexcept
{
    return m_totalOrdersSubmitted.load();
}

uint64_t BrokerGateway::totalFillsReceived() const noexcept
{
    return m_totalFillsReceived.load();
}

uint64_t BrokerGateway::totalApiErrors() const noexcept
{
    return m_totalApiErrors.load();
}

uint64_t BrokerGateway::reconciliationCount() const noexcept
{
    return m_reconciliationCount.load();
}

uint64_t BrokerGateway::faultDropsTriggered() const noexcept
{
    return m_faultDropCount.load();
}

uint64_t BrokerGateway::faultLatencySpikesTriggered() const noexcept
{
    return m_faultLatencyCount.load();
}

uint64_t BrokerGateway::faultDisconnectsTriggered() const noexcept
{
    return m_faultDisconnectCount.load();
}

uint64_t BrokerGateway::connectLifecycleCount() const noexcept
{
    return m_connectLifecycleCount.load();
}

uint64_t BrokerGateway::disconnectLifecycleCount() const noexcept
{
    return m_disconnectLifecycleCount.load();
}

uint64_t BrokerGateway::reconnectLifecycleCount() const noexcept
{
    return m_reconnectLifecycleCount.load();
}

// -- Private helpers ---------------------------------------------------------

double BrokerGateway::applySlippage(double price, bool isBuy) const noexcept
{
    const double slipFrac = SIMULATED_SLIPPAGE_BPS / 10000.0;
    return isBuy ? price * (1.0 + slipFrac) : price * (1.0 - slipFrac);
}
