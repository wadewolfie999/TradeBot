#include "OrderLifecycleStore.hpp"

#include <limits>
#include <utility>

namespace {

bool eventIsOlder(const OrderLifecycleRecord& record,
                  std::uint64_t timestampNs,
                  std::uint64_t sequence) noexcept
{
    if (sequence > 0 && record.lastSequence > 0) {
        return sequence <= record.lastSequence;
    }
    return timestampNs > 0 && record.lastTimestampNs > 0
        && timestampNs < record.lastTimestampNs;
}

} // namespace

bool OrderLifecycleStore::create(const OrderRequest& request)
{
    if (request.localOrderId == 0 || request.canonicalSymbol.empty()
        || !request.quantity.isPositive() || request.idempotencyKey.empty()
        || request.quantity.scale > Decimal64::MAX_SCALE) {
        return false;
    }
    if (m_records.contains(request.localOrderId)
        || m_appliedEventKeys.contains(request.idempotencyKey)) {
        return false;
    }

    OrderLifecycleRecord record;
    record.request = request;
    record.state = OrderLifecycleState::Created;
    record.preCancelState = OrderLifecycleState::Created;
    record.filledQuantity = Decimal64{0, request.quantity.scale};
    record.remainingQuantity = request.quantity;
    record.lastTimestampNs = request.timestampNs;
    record.lastSequence = request.sequence;
    appendAudit(record, request.idempotencyKey,
                request.timestampNs, request.sequence);
    m_records.emplace(request.localOrderId, std::move(record));
    m_appliedEventKeys.insert(request.idempotencyKey);
    return true;
}

LifecycleApplyResult OrderLifecycleStore::markRiskChecked(
    std::uint64_t localOrderId,
    const std::string& eventKey,
    std::uint64_t timestampNs,
    std::uint64_t sequence)
{
    return transition(localOrderId, OrderLifecycleState::RiskChecked,
                      eventKey, timestampNs, sequence);
}

LifecycleApplyResult OrderLifecycleStore::markSubmitted(
    std::uint64_t localOrderId,
    const std::string& eventKey,
    std::uint64_t timestampNs,
    std::uint64_t sequence)
{
    return transition(localOrderId, OrderLifecycleState::Submitted,
                      eventKey, timestampNs, sequence);
}

LifecycleApplyResult OrderLifecycleStore::applyAcknowledgement(
    const OrderAcknowledgement& ack)
{
    auto it = m_records.find(ack.localOrderId);
    if (it == m_records.end()) {
        return LifecycleApplyResult::UnknownOrder;
    }
    if (ack.eventKey.empty()) {
        return LifecycleApplyResult::InvalidEvent;
    }
    if (m_appliedEventKeys.contains(ack.eventKey)) {
        return LifecycleApplyResult::Duplicate;
    }
    if (eventIsOlder(it->second, ack.timestampNs, ack.sequence)) {
        return LifecycleApplyResult::InvalidEvent;
    }
    if (ack.accepted && ack.externalOrderId.empty()) {
        return LifecycleApplyResult::InvalidEvent;
    }

    const OrderLifecycleState next = ack.accepted
        ? OrderLifecycleState::Accepted
        : OrderLifecycleState::Rejected;
    if (!canTransition(it->second.state, next)) {
        return LifecycleApplyResult::InvalidTransition;
    }
    recordEventKey(ack.eventKey);
    it->second.state = next;
    it->second.externalOrderId = ack.externalOrderId;
    it->second.failure = ack.failure;
    it->second.reason = ack.reason;
    it->second.lastTimestampNs = ack.timestampNs;
    it->second.lastSequence = ack.sequence;
    appendAudit(it->second, ack.eventKey, ack.timestampNs, ack.sequence);
    return LifecycleApplyResult::Applied;
}

LifecycleApplyResult OrderLifecycleStore::applyExecution(
    const ExecutionEvent& execution)
{
    auto it = m_records.find(execution.localOrderId);
    if (it == m_records.end()) {
        return LifecycleApplyResult::UnknownOrder;
    }
    if (execution.eventKey.empty()) {
        return LifecycleApplyResult::InvalidEvent;
    }
    if (m_appliedEventKeys.contains(execution.eventKey)) {
        return LifecycleApplyResult::Duplicate;
    }
    if (eventIsOlder(it->second, execution.timestampNs, execution.sequence)
        || !validQuantities(it->second, execution.cumulativeFilledQuantity,
                            execution.remainingQuantity)
        || execution.cumulativeFilledQuantity.units
            < it->second.filledQuantity.units) {
        return LifecycleApplyResult::InvalidEvent;
    }

    const OrderLifecycleState next = execution.remainingQuantity.isZero()
        ? OrderLifecycleState::Filled
        : OrderLifecycleState::PartiallyFilled;
    if (!canTransition(it->second.state, next)) {
        return LifecycleApplyResult::InvalidTransition;
    }

    recordEventKey(execution.eventKey);
    it->second.state = next;
    it->second.externalOrderId = execution.externalOrderId;
    it->second.filledQuantity = execution.cumulativeFilledQuantity;
    it->second.remainingQuantity = execution.remainingQuantity;
    it->second.lastTimestampNs = execution.timestampNs;
    it->second.lastSequence = execution.sequence;
    appendAudit(it->second, execution.eventKey,
                execution.timestampNs, execution.sequence);
    return LifecycleApplyResult::Applied;
}

LifecycleApplyResult OrderLifecycleStore::requestCancel(const CancelRequest& request)
{
    auto it = m_records.find(request.localOrderId);
    if (it == m_records.end()) {
        return LifecycleApplyResult::UnknownOrder;
    }
    if (request.idempotencyKey.empty()) {
        return LifecycleApplyResult::InvalidEvent;
    }
    if (m_appliedEventKeys.contains(request.idempotencyKey)) {
        return LifecycleApplyResult::Duplicate;
    }
    if (eventIsOlder(it->second, request.timestampNs, request.sequence)
        || !canTransition(it->second.state, OrderLifecycleState::CancelRequested)) {
        return LifecycleApplyResult::InvalidTransition;
    }

    recordEventKey(request.idempotencyKey);
    it->second.preCancelState = it->second.state;
    it->second.state = OrderLifecycleState::CancelRequested;
    it->second.lastTimestampNs = request.timestampNs;
    it->second.lastSequence = request.sequence;
    appendAudit(it->second, request.idempotencyKey,
                request.timestampNs, request.sequence);
    return LifecycleApplyResult::Applied;
}

LifecycleApplyResult OrderLifecycleStore::applyCancelResult(
    const CancelResult& result)
{
    auto it = m_records.find(result.localOrderId);
    if (it == m_records.end()) {
        return LifecycleApplyResult::UnknownOrder;
    }
    if (result.eventKey.empty()) {
        return LifecycleApplyResult::InvalidEvent;
    }
    if (m_appliedEventKeys.contains(result.eventKey)) {
        return LifecycleApplyResult::Duplicate;
    }
    if (eventIsOlder(it->second, result.timestampNs, result.sequence)
        || it->second.state != OrderLifecycleState::CancelRequested) {
        return LifecycleApplyResult::InvalidTransition;
    }

    OrderLifecycleState next = OrderLifecycleState::Unknown;
    switch (result.disposition) {
        case CancelDisposition::Accepted:
        case CancelDisposition::AlreadyCanceled:
            next = OrderLifecycleState::Canceled;
            break;
        case CancelDisposition::AlreadyFilled:
            if (!it->second.remainingQuantity.isZero()) {
                return LifecycleApplyResult::InvalidEvent;
            }
            next = OrderLifecycleState::Filled;
            break;
        case CancelDisposition::Rejected:
            next = it->second.preCancelState;
            break;
        case CancelDisposition::Unknown:
            next = OrderLifecycleState::Unknown;
            break;
    }

    recordEventKey(result.eventKey);
    it->second.state = next;
    it->second.failure = result.failure;
    it->second.reason = result.reason;
    it->second.lastTimestampNs = result.timestampNs;
    it->second.lastSequence = result.sequence;
    appendAudit(it->second, result.eventKey,
                result.timestampNs, result.sequence);
    return LifecycleApplyResult::Applied;
}

LifecycleApplyResult OrderLifecycleStore::markTimeout(
    std::uint64_t localOrderId,
    const std::string& eventKey,
    std::uint64_t timestampNs,
    std::uint64_t sequence)
{
    return transition(localOrderId, OrderLifecycleState::Timeout,
                      eventKey, timestampNs, sequence);
}

LifecycleApplyResult OrderLifecycleStore::markUnknown(
    std::uint64_t localOrderId,
    const std::string& eventKey,
    FailureCategory failure,
    const std::string& reason,
    std::uint64_t timestampNs,
    std::uint64_t sequence)
{
    const auto applied = transition(localOrderId, OrderLifecycleState::Unknown,
                                    eventKey, timestampNs, sequence);
    if (applied == LifecycleApplyResult::Applied) {
        auto& record = m_records.at(localOrderId);
        record.failure = failure;
        record.reason = reason;
        auto& audit = record.auditHistory.back();
        audit.failure = failure;
        audit.reason = reason;
    }
    return applied;
}

LifecycleApplyResult OrderLifecycleStore::applyReconciliation(
    std::uint64_t localOrderId,
    OrderLifecycleState resolvedState,
    Decimal64 cumulativeFilled,
    Decimal64 remaining,
    const std::string& eventKey,
    std::uint64_t timestampNs,
    std::uint64_t sequence)
{
    auto it = m_records.find(localOrderId);
    if (it == m_records.end()) {
        return LifecycleApplyResult::UnknownOrder;
    }
    if (eventKey.empty()) {
        return LifecycleApplyResult::InvalidEvent;
    }
    if (m_appliedEventKeys.contains(eventKey)) {
        return LifecycleApplyResult::Duplicate;
    }
    if (eventIsOlder(it->second, timestampNs, sequence)
        || !validQuantities(it->second, cumulativeFilled, remaining)) {
        return LifecycleApplyResult::InvalidEvent;
    }
    if (!canTransition(it->second.state, OrderLifecycleState::Reconciled)) {
        return LifecycleApplyResult::InvalidTransition;
    }
    if (resolvedState != OrderLifecycleState::Filled
        && resolvedState != OrderLifecycleState::PartiallyFilled
        && resolvedState != OrderLifecycleState::Canceled
        && resolvedState != OrderLifecycleState::Expired
        && resolvedState != OrderLifecycleState::Rejected) {
        return LifecycleApplyResult::InvalidTransition;
    }

    recordEventKey(eventKey);
    it->second.state = OrderLifecycleState::Reconciled;
    it->second.reconciledState = resolvedState;
    it->second.filledQuantity = cumulativeFilled;
    it->second.remainingQuantity = remaining;
    it->second.lastTimestampNs = timestampNs;
    it->second.lastSequence = sequence;
    appendAudit(it->second, eventKey, timestampNs, sequence, resolvedState);
    return LifecycleApplyResult::Applied;
}

const OrderLifecycleRecord* OrderLifecycleStore::find(
    std::uint64_t localOrderId) const noexcept
{
    const auto it = m_records.find(localOrderId);
    return it == m_records.end() ? nullptr : &it->second;
}

LifecycleApplyResult OrderLifecycleStore::transition(
    std::uint64_t localOrderId,
    OrderLifecycleState next,
    const std::string& eventKey,
    std::uint64_t timestampNs,
    std::uint64_t sequence)
{
    auto it = m_records.find(localOrderId);
    if (it == m_records.end()) {
        return LifecycleApplyResult::UnknownOrder;
    }
    if (eventKey.empty()) {
        return LifecycleApplyResult::InvalidEvent;
    }
    if (m_appliedEventKeys.contains(eventKey)) {
        return LifecycleApplyResult::Duplicate;
    }
    if (eventIsOlder(it->second, timestampNs, sequence)) {
        return LifecycleApplyResult::InvalidEvent;
    }
    if (!canTransition(it->second.state, next)) {
        return LifecycleApplyResult::InvalidTransition;
    }

    recordEventKey(eventKey);
    it->second.state = next;
    it->second.lastTimestampNs = timestampNs;
    it->second.lastSequence = sequence;
    appendAudit(it->second, eventKey, timestampNs, sequence);
    return LifecycleApplyResult::Applied;
}

bool OrderLifecycleStore::recordEventKey(const std::string& eventKey)
{
    return !eventKey.empty() && m_appliedEventKeys.insert(eventKey).second;
}

void OrderLifecycleStore::appendAudit(
    OrderLifecycleRecord& record,
    const std::string& eventKey,
    std::uint64_t timestampNs,
    std::uint64_t sequence,
    std::optional<OrderLifecycleState> reconciledState)
{
    record.auditHistory.push_back(LifecycleAuditEntry{
        record.state,
        reconciledState,
        eventKey,
        record.failure,
        record.reason,
        timestampNs,
        sequence
    });
}

bool OrderLifecycleStore::isTerminal(OrderLifecycleState state) noexcept
{
    return state == OrderLifecycleState::Filled
        || state == OrderLifecycleState::Canceled
        || state == OrderLifecycleState::Expired
        || state == OrderLifecycleState::Rejected
        || state == OrderLifecycleState::Reconciled;
}

bool OrderLifecycleStore::canTransition(OrderLifecycleState from,
                                        OrderLifecycleState to) noexcept
{
    if (isTerminal(from)) {
        return false;
    }
    switch (to) {
        case OrderLifecycleState::RiskChecked:
            return from == OrderLifecycleState::Created;
        case OrderLifecycleState::Submitted:
            return from == OrderLifecycleState::RiskChecked;
        case OrderLifecycleState::Accepted:
        case OrderLifecycleState::Rejected:
            return from == OrderLifecycleState::Submitted;
        case OrderLifecycleState::PartiallyFilled:
        case OrderLifecycleState::Filled:
            return from == OrderLifecycleState::Submitted
                || from == OrderLifecycleState::Accepted
                || from == OrderLifecycleState::PartiallyFilled
                || from == OrderLifecycleState::CancelRequested
                || from == OrderLifecycleState::Timeout
                || from == OrderLifecycleState::Unknown;
        case OrderLifecycleState::CancelRequested:
            return from == OrderLifecycleState::Accepted
                || from == OrderLifecycleState::PartiallyFilled;
        case OrderLifecycleState::Canceled:
        case OrderLifecycleState::Expired:
            return from == OrderLifecycleState::Accepted
                || from == OrderLifecycleState::PartiallyFilled
                || from == OrderLifecycleState::CancelRequested
                || from == OrderLifecycleState::Unknown;
        case OrderLifecycleState::Timeout:
        case OrderLifecycleState::Unknown:
            return from == OrderLifecycleState::Submitted
                || from == OrderLifecycleState::Accepted
                || from == OrderLifecycleState::PartiallyFilled
                || from == OrderLifecycleState::CancelRequested
                || from == OrderLifecycleState::Timeout;
        case OrderLifecycleState::Reconciled:
            return from == OrderLifecycleState::Unknown
                || from == OrderLifecycleState::Timeout
                || from == OrderLifecycleState::CancelRequested
                || from == OrderLifecycleState::PartiallyFilled;
        case OrderLifecycleState::Created:
            return false;
    }
    return false;
}

bool OrderLifecycleStore::validQuantities(const OrderLifecycleRecord& record,
                                          Decimal64 cumulativeFilled,
                                          Decimal64 remaining) noexcept
{
    if (cumulativeFilled.scale != record.request.quantity.scale
        || remaining.scale != record.request.quantity.scale
        || cumulativeFilled.units < 0 || remaining.units < 0) {
        return false;
    }
    if (cumulativeFilled.units
        > std::numeric_limits<std::int64_t>::max() - remaining.units) {
        return false;
    }
    return cumulativeFilled.units + remaining.units
        == record.request.quantity.units;
}
