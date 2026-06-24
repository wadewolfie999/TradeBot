#pragma once

#include "BrokerAdapterContracts.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class LifecycleApplyResult : std::uint8_t {
    Applied,
    Duplicate,
    UnknownOrder,
    InvalidEvent,
    InvalidTransition
};

struct LifecycleAuditEntry {
    OrderLifecycleState state{OrderLifecycleState::Created};
    std::optional<OrderLifecycleState> reconciledState;
    std::string eventKey;
    FailureCategory failure{FailureCategory::None};
    std::string reason;
    std::uint64_t timestampNs{0};
    std::uint64_t sequence{0};
};

struct OrderLifecycleRecord {
    OrderRequest request;
    OrderLifecycleState state{OrderLifecycleState::Created};
    OrderLifecycleState preCancelState{OrderLifecycleState::Created};
    std::string externalOrderId;
    Decimal64 filledQuantity;
    Decimal64 remainingQuantity;
    FailureCategory failure{FailureCategory::None};
    std::string reason;
    std::uint64_t lastTimestampNs{0};
    std::uint64_t lastSequence{0};
    std::optional<OrderLifecycleState> reconciledState;
    std::vector<LifecycleAuditEntry> auditHistory;
};

class OrderLifecycleStore {
public:
    bool create(const OrderRequest& request);
    LifecycleApplyResult markRiskChecked(std::uint64_t localOrderId,
                                         const std::string& eventKey,
                                         std::uint64_t timestampNs,
                                         std::uint64_t sequence);
    LifecycleApplyResult markSubmitted(std::uint64_t localOrderId,
                                       const std::string& eventKey,
                                       std::uint64_t timestampNs,
                                       std::uint64_t sequence);
    LifecycleApplyResult applyAcknowledgement(const OrderAcknowledgement& ack);
    LifecycleApplyResult applyExecution(const ExecutionEvent& execution);
    LifecycleApplyResult requestCancel(const CancelRequest& request);
    LifecycleApplyResult applyCancelResult(const CancelResult& result);
    LifecycleApplyResult markTimeout(std::uint64_t localOrderId,
                                     const std::string& eventKey,
                                     std::uint64_t timestampNs,
                                     std::uint64_t sequence);
    LifecycleApplyResult markUnknown(std::uint64_t localOrderId,
                                     const std::string& eventKey,
                                     FailureCategory failure,
                                     const std::string& reason,
                                     std::uint64_t timestampNs,
                                     std::uint64_t sequence);
    LifecycleApplyResult applyReconciliation(std::uint64_t localOrderId,
                                             OrderLifecycleState resolvedState,
                                             Decimal64 cumulativeFilled,
                                             Decimal64 remaining,
                                             const std::string& eventKey,
                                             std::uint64_t timestampNs,
                                             std::uint64_t sequence);

    const OrderLifecycleRecord* find(std::uint64_t localOrderId) const noexcept;
    std::size_t size() const noexcept { return m_records.size(); }
    std::size_t appliedEventCount() const noexcept { return m_appliedEventKeys.size(); }

private:
    LifecycleApplyResult transition(std::uint64_t localOrderId,
                                    OrderLifecycleState next,
                                    const std::string& eventKey,
                                    std::uint64_t timestampNs,
                                    std::uint64_t sequence);
    bool recordEventKey(const std::string& eventKey);
    static void appendAudit(OrderLifecycleRecord& record,
                            const std::string& eventKey,
                            std::uint64_t timestampNs,
                            std::uint64_t sequence,
                            std::optional<OrderLifecycleState> reconciledState = std::nullopt);
    static bool isTerminal(OrderLifecycleState state) noexcept;
    static bool canTransition(OrderLifecycleState from,
                              OrderLifecycleState to) noexcept;
    static bool validQuantities(const OrderLifecycleRecord& record,
                                Decimal64 cumulativeFilled,
                                Decimal64 remaining) noexcept;

    std::unordered_map<std::uint64_t, OrderLifecycleRecord> m_records;
    std::unordered_set<std::string> m_appliedEventKeys;
};
