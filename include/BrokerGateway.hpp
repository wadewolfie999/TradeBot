#pragma once
// BrokerGateway.hpp -- REST/WSS broker execution gateway.
//
// Phase 12 (MOP-20260322-07, Workstream 7.3):
//   Implements the ExecutionAdapter interface for PAPER and LIVE modes.
//   In PAPER mode all broker calls are mocked locally (no network I/O) while
//   the accounting is delegated to the existing PortfolioManager so PnL,
//   fees, and equity tracking remain consistent with the BACKTEST path.
//   In LIVE mode the same interface would route orders to a real broker via
//   REST endpoints and receive fill confirmations over WSS.
//
// Responsibilities
// ----------------
//   1. Order placement  -- submitOrder() serializes the order to a REST
//      payload and simulates (PAPER) or sends (LIVE) the HTTP request.
//   2. Order cancellation -- cancelOrder() routes the REST cancel request.
//   3. Fill confirmations -- WSS fill events are parsed and forwarded to
//      PortfolioManager::recordFill() (simulation: injected via
//      simulateFillConfirmation()).
//   4. Portfolio reconciliation -- a periodic sync loop compares local
//      PortfolioManager state with broker-reported balances and logs deltas.
//   5. Actual slippage feed-back -- real fill prices are pushed back to the
//      ExecutionEngine so live PnL and total expectancy (E_total) reflect
//      true transaction costs rather than modeled estimates.
//
// PAPER-mode accounting
// ----------------------
//   BrokerGateway delegates all position/PnL accounting to the shared
//   PortfolioManager instance so the live execution path shares the same
//   equity and drawdown tracking as BACKTEST.
#include "PortfolioManager.hpp"
#include "SystemConfig.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <cstdint>

// -- Fill record returned by the gateway after a simulated/live fill ---------

struct BrokerFill {
    uint64_t    orderId{0};
    std::string symbol;
    bool        isBuy{true};
    double      requestedPrice{0.0};
    double      fillPrice{0.0};      // actual price (may include live slippage)
    double      quantity{0.0};       // requested order quantity
    double      filledQuantity{0.0}; // quantity filled in this event
    double      remainingQuantity{0.0};
    bool        partialFill{false};
    double      feePaid{0.0};
    uint64_t    fillTimestamp{0};
    std::string strategyId;
    bool        success{false};
    std::string errorMessage;
};

// -- BrokerAccount: reconciliation snapshot ----------------------------------

struct BrokerAccount {
    double      cashBalance{0.0};    // broker-reported free cash
    double      totalEquity{0.0};    // broker-reported total portfolio value
    uint32_t    openPositions{0};    // broker-reported open position count
    uint64_t    snapshotTimestamp{0};
};

// -- BrokerGateway -----------------------------------------------------------

class BrokerGateway {
public:
    struct FaultInjectorConfig {
        bool     enabled{false};
        uint32_t dropEveryN{0};         // fail every Nth order with simulated packet loss
        uint32_t latencySpikeEveryN{0}; // apply latency spike every Nth order
        uint32_t latencySpikeMs{0};
        uint32_t disconnectEveryN{0};   // force connection drop every Nth order
    };

    // Callback fired on every fill (real or simulated).
    using FillCallback = std::function<void(const BrokerFill&)>;

    // Construct with the shared config and a PortfolioManager reference that
    // serves as the local accounting book in PAPER mode.
    BrokerGateway(const SystemConfig& cfg,
                  PortfolioManager&   portfolio) noexcept;

    ~BrokerGateway() = default;

    // -- Connection lifecycle -----------------------------------------------

    // Establish REST session and WSS fill stream (no-op in BACKTEST; mock in PAPER).
    void connect();

    // Gracefully close WSS fill stream and flush any pending confirmations.
    void disconnect() noexcept;

    bool isConnected() const noexcept;

    // -- Order management ---------------------------------------------------

    // Place a market or limit order.  In PAPER mode: synchronously fills at
    // `requestedPrice` ± simulated slippage and calls PortfolioManager.
    // Returns a BrokerFill describing the outcome.
    BrokerFill submitOrder(const std::string& symbol,
                           bool               isBuy,
                           double             quantity,
                           double             requestedPrice,
                           uint64_t           timestamp,
                           const std::string& strategyId = "");

    // Cancel a pending order by ID.
    // Returns true if the cancellation was acknowledged.
    bool cancelOrder(uint64_t orderId);

    // -- Portfolio reconciliation -------------------------------------------

    // Compare local PortfolioManager state with broker-reported account
    // snapshot and log any discrepancies.  Called periodically by the main loop.
    void reconcile(uint64_t nowTimestamp);

    // Inject a simulated broker account snapshot (for PAPER / test harness).
    void injectBrokerSnapshot(const BrokerAccount& snap) noexcept;

    // Latest reconciliation snapshot.
    BrokerAccount lastBrokerSnapshot() const noexcept;

    // -- Fill feed-back to ExecutionEngine ----------------------------------

    // Register a callback that is invoked on every fill so the ExecutionEngine
    // can update live PnL and E_total with actual transaction costs.
    void setFillCallback(FillCallback cb) noexcept;
    void addFillCallback(FillCallback cb) noexcept;

    // -- Simulation harness -------------------------------------------------

    // Inject a fill confirmation directly (for PAPER-mode / unit tests).
    void simulateFillConfirmation(const BrokerFill& fill);

    // Error injection: force the next submitOrder() call to return a failure.
    // `errorMessage` is the simulated broker error string.
    void injectNextOrderError(const std::string& errorMessage) noexcept;

    // Partial-fill injection: force the next submitOrder() to emit a partial
    // fill with the provided ratio in (0, 1). Remaining quantity stays pending.
    void injectNextPartialFill(double fillRatio) noexcept;

    // Deterministic network fault injector for local resilience validation.
    void setFaultInjectorConfig(const FaultInjectorConfig& cfg) noexcept;
    FaultInjectorConfig faultInjectorConfig() const noexcept;

    // -- Metrics (observable for circuit-breaker tests) ---------------------

    uint64_t totalOrdersSubmitted()  const noexcept;
    uint64_t totalFillsReceived()    const noexcept;
    uint64_t totalApiErrors()        const noexcept;
    uint64_t reconciliationCount()   const noexcept;
    uint64_t faultDropsTriggered()   const noexcept;
    uint64_t faultLatencySpikesTriggered() const noexcept;
    uint64_t faultDisconnectsTriggered() const noexcept;
    uint64_t connectLifecycleCount() const noexcept;
    uint64_t disconnectLifecycleCount() const noexcept;
    uint64_t reconnectLifecycleCount() const noexcept;

private:
    // Apply simulated slippage to a price.
    // isBuy=true increases the price (adverse fill for buyer).
    double applySlippage(double price, bool isBuy) const noexcept;

    const SystemConfig& m_cfg;
    PortfolioManager&   m_portfolio;

    mutable std::mutex  m_mutex;
    std::atomic<bool>   m_connected{false};

    FillCallback        m_fillCallback;
    std::vector<FillCallback> m_fillCallbacks;

    // Order ID generator.
    std::atomic<uint64_t> m_nextOrderId{1000};

    // Counters for circuit-breaker visibility.
    std::atomic<uint64_t> m_totalOrdersSubmitted{0};
    std::atomic<uint64_t> m_totalFillsReceived{0};
    std::atomic<uint64_t> m_totalApiErrors{0};
    std::atomic<uint64_t> m_reconciliationCount{0};
    std::atomic<uint64_t> m_connectLifecycleCount{0};
    std::atomic<uint64_t> m_disconnectLifecycleCount{0};
    std::atomic<uint64_t> m_reconnectLifecycleCount{0};
    std::atomic<uint64_t> m_faultDropCount{0};
    std::atomic<uint64_t> m_faultLatencyCount{0};
    std::atomic<uint64_t> m_faultDisconnectCount{0};

    // Latest broker-reported account snapshot.
    BrokerAccount m_lastSnapshot;

    // Error injection state.
    bool        m_injectError{false};
    std::string m_injectedErrorMsg;
    bool        m_injectPartialFill{false};
    double      m_partialFillRatio{1.0};
    FaultInjectorConfig m_faultCfg;
    uint64_t    m_faultSequence{0};
    std::unordered_map<uint64_t, double> m_pendingRemainingQty;
};
