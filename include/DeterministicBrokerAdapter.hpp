#pragma once

#include "IBrokerAdapter.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class DeterministicBrokerAdapter final : public IBrokerAdapter {
public:
    struct FaultConfig {
        bool enabled{false};
        std::uint32_t dropEveryN{0};
        std::uint32_t latencySpikeEveryN{0};
        std::uint32_t latencySpikeMs{0};
        std::uint32_t disconnectEveryN{0};
    };

    void setAcknowledgementCallback(AcknowledgementCallback callback) override;
    void setExecutionCallback(ExecutionCallback callback) override;
    void setCancelCallback(CancelCallback callback) override;
    void setHealthCallback(HealthCallback callback) override;

    bool connect() override;
    void disconnect() noexcept override;
    bool isConnected() const noexcept override;
    bool submit(const NormalizedOrder& order) override;
    bool cancel(const CancelRequest& request) override;
    ReconciliationSnapshot reconcile(std::uint64_t timestampNs) override;

    AdapterHealthEvent health() const override;
    std::optional<AccountSnapshot> accountSnapshot() const override;
    std::optional<InstrumentSpec> instrumentSpec(
        const std::string& canonicalSymbol) const override;

    void setInstrumentSpec(const InstrumentSpec& spec);
    void setAccountSnapshot(const AccountSnapshot& snapshot);
    void injectNextError(std::string reason);
    void injectNextPartialFill(double ratio) noexcept;
    void setFaultConfig(const FaultConfig& config) noexcept;
    FaultConfig faultConfig() const noexcept;

    std::uint64_t faultDropsTriggered() const noexcept;
    std::uint64_t faultLatencySpikesTriggered() const noexcept;
    std::uint64_t faultDisconnectsTriggered() const noexcept;
    std::uint64_t connectCount() const noexcept;
    std::uint64_t disconnectCount() const noexcept;
    std::uint64_t reconnectCount() const noexcept;

private:
    void publishHealth(AdapterHealthState state,
                       std::uint64_t timestampNs,
                       std::uint32_t latencyMs = 0,
                       FailureCategory failure = FailureCategory::None,
                       const std::string& reason = {});

    mutable std::mutex m_mutex;
    bool m_connected{false};
    bool m_everConnected{false};
    std::uint64_t m_sequence{0};
    std::uint64_t m_submitCount{0};
    std::uint64_t m_faultDrops{0};
    std::uint64_t m_faultLatencySpikes{0};
    std::uint64_t m_faultDisconnects{0};
    std::uint64_t m_connectCount{0};
    std::uint64_t m_disconnectCount{0};
    std::uint64_t m_reconnectCount{0};
    bool m_injectError{false};
    std::string m_injectedError;
    std::optional<double> m_partialFillRatio;
    FaultConfig m_faultConfig;
    AdapterHealthEvent m_health;
    std::optional<AccountSnapshot> m_accountSnapshot;
    std::unordered_map<std::string, InstrumentSpec> m_instruments;
    std::unordered_map<std::uint64_t, NormalizedOrder> m_pendingOrders;

    AcknowledgementCallback m_acknowledgementCallback;
    ExecutionCallback m_executionCallback;
    CancelCallback m_cancelCallback;
    HealthCallback m_healthCallback;
};
