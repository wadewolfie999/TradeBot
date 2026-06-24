#include "BrokerAdapterContracts.hpp"
#include "OrderLifecycleStore.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

OrderRequest makeRequest(std::uint64_t id, std::int64_t quantityUnits = 100)
{
    OrderRequest request;
    request.localOrderId = id;
    request.canonicalSymbol = "SYNTH-USD";
    request.side = OrderSide::Buy;
    request.type = OrderType::Market;
    request.quantity = Decimal64{quantityUnits, 2};
    request.sourceId = "phase22-test";
    request.timestampNs = 1'000;
    request.sequence = 1;
    request.idempotencyKey = "create-" + std::to_string(id);
    return request;
}

void advanceToAccepted(OrderLifecycleStore& store, std::uint64_t id)
{
    require(store.markRiskChecked(id, "risk-" + std::to_string(id), 2'000, 2)
                == LifecycleApplyResult::Applied,
            "risk transition should apply");
    require(store.markSubmitted(id, "submit-" + std::to_string(id), 3'000, 3)
                == LifecycleApplyResult::Applied,
            "submit transition should apply");

    OrderAcknowledgement ack;
    ack.localOrderId = id;
    ack.externalOrderId = "external-" + std::to_string(id);
    ack.accepted = true;
    ack.timestampNs = 4'000;
    ack.sequence = 4;
    ack.eventKey = "ack-" + std::to_string(id);
    require(store.applyAcknowledgement(ack) == LifecycleApplyResult::Applied,
            "acknowledgement should apply");
}

void testDecimal64()
{
    const auto value = Decimal64::fromDouble(12.345, 2);
    require(value.has_value(), "rounded Decimal64 should be created");
    require(value->units == 1235 && value->scale == 2,
            "nearest rounding should be explicit");
    require(std::fabs(value->toDouble() - 12.35) < 1e-12,
            "Decimal64 roundtrip mismatch");

    require(!Decimal64::fromDouble(12.345, 2,
                                   DecimalRounding::RejectUnaligned).has_value(),
            "unaligned decimal should be rejected");
    const auto truncated = Decimal64::fromDouble(-12.349, 2,
                                                  DecimalRounding::TowardZero);
    require(truncated.has_value() && truncated->units == -1234,
            "toward-zero rounding mismatch");
    require(!Decimal64::fromDouble(std::numeric_limits<double>::infinity(), 2)
                 .has_value(),
            "non-finite decimal should be rejected");
    require(!Decimal64::fromDouble(1.0, Decimal64::MAX_SCALE + 1).has_value(),
            "unsupported scale should be rejected");
}

void testAcknowledgementDoesNotFill()
{
    OrderLifecycleStore store;
    require(store.create(makeRequest(1)), "request creation failed");
    advanceToAccepted(store, 1);
    const auto* record = store.find(1);
    require(record && record->state == OrderLifecycleState::Accepted,
            "acknowledgement must not imply fill");
    require(record->filledQuantity.units == 0
                && record->remainingQuantity.units == 100,
            "acknowledgement must not change quantities");
    require(record->auditHistory.size() == 4,
            "creation and every transition must be audited");
}

void testPartialAndFinalFillDeduplication()
{
    OrderLifecycleStore store;
    require(store.create(makeRequest(2)), "request creation failed");
    advanceToAccepted(store, 2);

    ExecutionEvent partial;
    partial.localOrderId = 2;
    partial.externalOrderId = "external-2";
    partial.cumulativeFilledQuantity = Decimal64{40, 2};
    partial.remainingQuantity = Decimal64{60, 2};
    partial.fillPrice = Decimal64{10000, 2};
    partial.fee = Decimal64{1, 2};
    partial.timestampNs = 5'000;
    partial.sequence = 5;
    partial.eventKey = "fill-2-1";
    require(store.applyExecution(partial) == LifecycleApplyResult::Applied,
            "partial fill should apply");
    require(store.applyExecution(partial) == LifecycleApplyResult::Duplicate,
            "duplicate fill must be rejected");

    ExecutionEvent finalFill = partial;
    finalFill.cumulativeFilledQuantity = Decimal64{100, 2};
    finalFill.remainingQuantity = Decimal64{0, 2};
    finalFill.timestampNs = 6'000;
    finalFill.sequence = 6;
    finalFill.eventKey = "fill-2-2";
    require(store.applyExecution(finalFill) == LifecycleApplyResult::Applied,
            "final fill should apply");
    const auto* record = store.find(2);
    require(record && record->state == OrderLifecycleState::Filled,
            "order should be filled");
}

void testCancelUnknownRequiresReconciliation()
{
    OrderLifecycleStore store;
    require(store.create(makeRequest(3)), "request creation failed");
    advanceToAccepted(store, 3);

    CancelRequest cancel;
    cancel.localOrderId = 3;
    cancel.externalOrderId = "external-3";
    cancel.timestampNs = 5'000;
    cancel.sequence = 5;
    cancel.idempotencyKey = "cancel-request-3";
    require(store.requestCancel(cancel) == LifecycleApplyResult::Applied,
            "cancel request should apply");

    CancelResult result;
    result.localOrderId = 3;
    result.disposition = CancelDisposition::Unknown;
    result.failure = FailureCategory::Timeout;
    result.reason = "synthetic timeout";
    result.timestampNs = 6'000;
    result.sequence = 6;
    result.eventKey = "cancel-result-3";
    require(store.applyCancelResult(result) == LifecycleApplyResult::Applied,
            "unknown cancel should apply");
    require(store.find(3)->state == OrderLifecycleState::Unknown,
            "unknown cancel must remain unknown");

    require(store.applyReconciliation(3, OrderLifecycleState::Canceled,
                                      Decimal64{0, 2}, Decimal64{100, 2},
                                      "reconcile-3", 7'000, 7)
                == LifecycleApplyResult::Applied,
            "reconciliation should resolve unknown cancel");
    require(store.find(3)->state == OrderLifecycleState::Reconciled,
            "reconciled state expected");
    require(store.find(3)->reconciledState == OrderLifecycleState::Canceled,
            "reconciled outcome must be explicit");
    require(store.find(3)->auditHistory.back().reconciledState
                == OrderLifecycleState::Canceled,
            "reconciled outcome must be retained in audit history");
}

void testInvalidAndStaleEvents()
{
    OrderLifecycleStore store;
    require(store.create(makeRequest(4)), "request creation failed");
    require(store.markSubmitted(4, "invalid-submit", 2'000, 2)
                == LifecycleApplyResult::InvalidTransition,
            "submit before risk check must fail");
    advanceToAccepted(store, 4);

    ExecutionEvent malformed;
    malformed.localOrderId = 4;
    malformed.cumulativeFilledQuantity = Decimal64{30, 2};
    malformed.remainingQuantity = Decimal64{80, 2};
    malformed.timestampNs = 5'000;
    malformed.sequence = 5;
    malformed.eventKey = "malformed-4";
    require(store.applyExecution(malformed) == LifecycleApplyResult::InvalidEvent,
            "inconsistent quantities must fail");

    OrderAcknowledgement stale;
    stale.localOrderId = 4;
    stale.accepted = false;
    stale.timestampNs = 2'500;
    stale.sequence = 2;
    stale.eventKey = "stale-4";
    require(store.applyAcknowledgement(stale) == LifecycleApplyResult::InvalidEvent,
            "stale event must fail");

    ExecutionEvent sameSequence;
    sameSequence.localOrderId = 4;
    sameSequence.externalOrderId = "external-4";
    sameSequence.cumulativeFilledQuantity = Decimal64{20, 2};
    sameSequence.remainingQuantity = Decimal64{80, 2};
    sameSequence.timestampNs = 5'000;
    sameSequence.sequence = 4;
    sameSequence.eventKey = "same-sequence-4";
    require(store.applyExecution(sameSequence) == LifecycleApplyResult::InvalidEvent,
            "reused sequence must fail");

    require(store.applyReconciliation(4, OrderLifecycleState::Canceled,
                                      Decimal64{0, 2}, Decimal64{100, 2},
                                      "premature-reconcile-4", 6'000, 6)
                == LifecycleApplyResult::InvalidTransition,
            "reconciliation from a known accepted state must fail");
}

} // namespace

int main()
{
    testDecimal64();
    testAcknowledgementDoesNotFill();
    testPartialAndFinalFillDeduplication();
    testCancelUnknownRequiresReconciliation();
    testInvalidAndStaleEvents();
    std::cout << "phase22_tests: PASS\n";
    return 0;
}
