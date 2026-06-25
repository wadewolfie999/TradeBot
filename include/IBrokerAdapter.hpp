#pragma once

#include "BrokerAdapterContracts.hpp"

#include <functional>
#include <optional>

class IBrokerAdapter {
public:
    using AcknowledgementCallback = std::function<void(const OrderAcknowledgement&)>;
    using ExecutionCallback = std::function<void(const ExecutionEvent&)>;
    using CancelCallback = std::function<void(const CancelResult&)>;
    using HealthCallback = std::function<void(const AdapterHealthEvent&)>;

    virtual ~IBrokerAdapter() = default;

    virtual void setAcknowledgementCallback(AcknowledgementCallback callback) = 0;
    virtual void setExecutionCallback(ExecutionCallback callback) = 0;
    virtual void setCancelCallback(CancelCallback callback) = 0;
    virtual void setHealthCallback(HealthCallback callback) = 0;

    virtual bool connect() = 0;
    virtual void disconnect() noexcept = 0;
    virtual bool isConnected() const noexcept = 0;

    // A true return means only that the adapter accepted the request for
    // processing. Acknowledgement and execution remain separate events.
    virtual bool submit(const NormalizedOrder& order) = 0;
    virtual bool cancel(const CancelRequest& request) = 0;
    virtual ReconciliationSnapshot reconcile(std::uint64_t timestampNs) = 0;

    virtual AdapterHealthEvent health() const = 0;
    virtual std::optional<AccountSnapshot> accountSnapshot() const = 0;
    virtual std::optional<InstrumentSpec> instrumentSpec(
        const std::string& canonicalSymbol) const = 0;
};
