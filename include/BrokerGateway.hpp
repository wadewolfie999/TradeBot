#pragma once

#include "BrokerAdapterContracts.hpp"
#include "DeterministicBrokerAdapter.hpp"
#include "IBrokerAdapter.hpp"
#include "OrderLifecycleStore.hpp"
#include "PortfolioManager.hpp"
#include "SystemConfig.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// Compatibility fill used by the existing ExecutionEngine until its Phase 22
// lifecycle-event migration lands in the next sequential PR.
struct BrokerFill {
    std::uint64_t orderId{0};
    std::string symbol;
    bool isBuy{true};
    double requestedPrice{0.0};
    double fillPrice{0.0};
    double quantity{0.0};
    double filledQuantity{0.0};
    double remainingQuantity{0.0};
    bool partialFill{false};
    double feePaid{0.0};
    std::uint64_t fillTimestamp{0};
    std::string strategyId;
    bool success{false};
    std::string errorMessage;
};

struct BrokerAccount {
    double cashBalance{0.0};
    double totalEquity{0.0};
    std::uint32_t openPositions{0};
    std::uint64_t snapshotTimestamp{0};
};

struct GatewayDispatchResult {
    bool dispatched{false};
    std::uint64_t localOrderId{0};
    FailureCategory failure{FailureCategory::None};
    std::string reason;
};

class BrokerGateway {
public:
    using FaultInjectorConfig = DeterministicBrokerAdapter::FaultConfig;
    using FillCallback = std::function<void(const BrokerFill&)>;
    using AcknowledgementCallback = std::function<void(const OrderAcknowledgement&)>;
    using ExecutionCallback = std::function<void(const ExecutionEvent&)>;
    using CancelCallback = std::function<void(const CancelResult&)>;
    using HealthCallback = std::function<void(const AdapterHealthEvent&)>;

    BrokerGateway(const SystemConfig& config,
                  PortfolioManager& portfolio);
    BrokerGateway(const SystemConfig& config,
                  PortfolioManager& portfolio,
                  std::unique_ptr<IBrokerAdapter> adapter,
                  bool liveAdapterApproved = false);
    ~BrokerGateway();

    void connect();
    void disconnect() noexcept;
    bool isConnected() const noexcept;

    void setSymbolAlias(std::string canonicalSymbol, std::string executionAlias);
    void setInstrumentSpec(const InstrumentSpec& spec);
    std::optional<NormalizedOrder> normalizeOrder(
        const OrderRequest& request,
        std::string* rejectionReason = nullptr) const;
    GatewayDispatchResult dispatchOrder(const NormalizedOrder& order,
                                        const RiskDecision& finalRiskDecision);
    bool requestCancel(const CancelRequest& request);
    ReconciliationSnapshot reconciliationSnapshot(std::uint64_t timestampNs);

    std::optional<OrderLifecycleRecord> orderLifecycle(
        std::uint64_t localOrderId) const noexcept;
    AdapterHealthEvent adapterHealth() const;

    void setAcknowledgementCallback(AcknowledgementCallback callback) noexcept;
    void setExecutionCallback(ExecutionCallback callback) noexcept;
    void setCancelCallback(CancelCallback callback) noexcept;
    void setHealthCallback(HealthCallback callback) noexcept;

    // Compatibility API. New Phase 22 code must use normalizeOrder(),
    // dispatchOrder(), lifecycle callbacks, and requestCancel().
    BrokerFill submitOrder(const std::string& symbol,
                           bool isBuy,
                           double quantity,
                           double requestedPrice,
                           std::uint64_t timestamp,
                           const std::string& strategyId = "");
    bool cancelOrder(std::uint64_t orderId);
    void reconcile(std::uint64_t nowTimestamp);
    void injectBrokerSnapshot(const BrokerAccount& snapshot) noexcept;
    BrokerAccount lastBrokerSnapshot() const noexcept;
    void setFillCallback(FillCallback callback) noexcept;
    void addFillCallback(FillCallback callback) noexcept;
    void simulateFillConfirmation(const BrokerFill& fill);
    void injectNextOrderError(const std::string& errorMessage) noexcept;
    void injectNextPartialFill(double fillRatio) noexcept;
    void setFaultInjectorConfig(const FaultInjectorConfig& config) noexcept;
    FaultInjectorConfig faultInjectorConfig() const noexcept;

    std::uint64_t totalOrdersSubmitted() const noexcept;
    std::uint64_t totalFillsReceived() const noexcept;
    std::uint64_t totalApiErrors() const noexcept;
    std::uint64_t reconciliationCount() const noexcept;
    std::uint64_t faultDropsTriggered() const noexcept;
    std::uint64_t faultLatencySpikesTriggered() const noexcept;
    std::uint64_t faultDisconnectsTriggered() const noexcept;
    std::uint64_t connectLifecycleCount() const noexcept;
    std::uint64_t disconnectLifecycleCount() const noexcept;
    std::uint64_t reconnectLifecycleCount() const noexcept;

private:
    struct LegacyOrderContext {
        std::string symbol;
        bool isBuy{true};
        double requestedPrice{0.0};
        double requestedQuantity{0.0};
        std::string strategyId;
    };

    bool adapterAuthorized() const noexcept;
    DeterministicBrokerAdapter* deterministicAdapter() noexcept;
    const DeterministicBrokerAdapter* deterministicAdapter() const noexcept;
    void bindAdapterCallbacks();
    void handleAcknowledgement(const OrderAcknowledgement& acknowledgement);
    void handleExecution(const ExecutionEvent& execution);
    void handleCancel(const CancelResult& result);
    void handleHealth(const AdapterHealthEvent& health);
    void publishLegacyFill(const BrokerFill& fill);

    const SystemConfig& m_config;
    PortfolioManager& m_portfolio;
    std::unique_ptr<IBrokerAdapter> m_adapter;
    bool m_liveAdapterApproved{false};

    mutable std::mutex m_mutex;
    OrderLifecycleStore m_lifecycle;
    std::unordered_map<std::string, std::string> m_symbolAliases;
    std::unordered_map<std::string, InstrumentSpec> m_instrumentSpecs;
    std::unordered_map<std::uint64_t, LegacyOrderContext> m_legacyContexts;
    std::unordered_map<std::uint64_t, BrokerFill> m_lastLegacyFills;
    BrokerAccount m_lastBrokerSnapshot;
    ReconciliationSnapshot m_lastReconciliation;
    AdapterHealthEvent m_health;

    FillCallback m_fillCallback;
    std::vector<FillCallback> m_fillCallbacks;
    AcknowledgementCallback m_acknowledgementCallback;
    ExecutionCallback m_executionCallback;
    CancelCallback m_cancelCallback;
    HealthCallback m_healthCallback;

    std::atomic<std::uint64_t> m_nextOrderId{1000};
    std::atomic<std::uint64_t> m_totalOrdersSubmitted{0};
    std::atomic<std::uint64_t> m_totalFillsReceived{0};
    std::atomic<std::uint64_t> m_totalApiErrors{0};
    std::atomic<std::uint64_t> m_reconciliationCount{0};
};
