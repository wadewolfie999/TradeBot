#include "BrokerAdapterContracts.hpp"
#include "BrokerGateway.hpp"
#include "OrderLifecycleStore.hpp"
#include "PortfolioManager.hpp"
#include "SystemConfig.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
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
    request.type = BrokerOrderType::Market;
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

InstrumentSpec makeInstrumentSpec()
{
    InstrumentSpec spec;
    spec.version = 7;
    spec.canonicalSymbol = "SYNTH-USD";
    spec.executionAlias = "SYNTHUSD";
    spec.tickSize = Decimal64{1, 2};
    spec.contractSize = Decimal64{1, 0};
    spec.minimumQuantity = Decimal64{10, 2};
    spec.maximumQuantity = Decimal64{200, 2};
    spec.quantityStep = Decimal64{5, 2};
    spec.complete = true;
    return spec;
}

void testGatewayNormalizationAndLifecycleEvents()
{
    SystemConfig config;
    config.mode = SystemMode::PAPER;
    PortfolioManager portfolio;
    BrokerGateway gateway(config, portfolio);
    gateway.setInstrumentSpec(makeInstrumentSpec());
    gateway.setSymbolAlias("SYNTH-USD", "SIM.SYNTHUSD");

    int acknowledgementCount = 0;
    int executionCount = 0;
    gateway.setAcknowledgementCallback(
        [&](const OrderAcknowledgement& acknowledgement) {
            require(acknowledgement.accepted,
                    "deterministic acknowledgement should accept");
            ++acknowledgementCount;
        });
    gateway.setExecutionCallback([&](const ExecutionEvent&) {
        ++executionCount;
    });
    gateway.connect();
    require(gateway.isConnected(), "PAPER adapter should connect locally");
    require(gateway.adapterHealth().state == AdapterHealthState::Connected,
            "connected health state expected");

    auto request = makeRequest(10, 1239);
    request.quantity = Decimal64{1239, 3};
    request.referencePrice = Decimal64{100123, 3};
    std::string rejection;
    const auto normalized = gateway.normalizeOrder(request, &rejection);
    require(normalized.has_value(), "valid order should normalize");
    require(normalized->normalizedQuantity == Decimal64{120, 2},
            "quantity must round down to the configured step");
    require(normalized->normalizedQuantity.toDouble()
                <= request.quantity.toDouble(),
            "normalization must not increase exposure");
    require(normalized->normalizedReferencePrice == Decimal64{10012, 2},
            "price must normalize to tick size");
    require(normalized->executionSymbol == "SIM.SYNTHUSD",
            "gateway-owned symbol alias must be applied");
    require(normalized->instrumentVersion == 7,
            "instrument metadata version must be retained");

    RiskDecision denied;
    denied.allowed = false;
    denied.failure = FailureCategory::Validation;
    denied.action = RuleBreachAction::Halt;
    denied.reason = "synthetic risk rejection";
    require(!gateway.dispatchOrder(*normalized, denied).dispatched,
            "gateway must not bypass a rejected final risk decision");
    require(!gateway.orderLifecycle(10).has_value(),
            "risk-rejected order must not enter adapter lifecycle");

    RiskDecision allowed;
    allowed.allowed = true;
    allowed.action = RuleBreachAction::Warn;
    const auto dispatched = gateway.dispatchOrder(*normalized, allowed);
    require(dispatched.dispatched, "normalized PAPER order should dispatch");
    require(acknowledgementCount == 1,
            "acknowledgement must be emitted exactly once");
    require(executionCount == 1,
            "execution must be emitted separately exactly once");
    const auto lifecycle = gateway.orderLifecycle(10);
    require(lifecycle.has_value()
                && lifecycle->state == OrderLifecycleState::Filled,
            "deterministic execution should reach filled state");
    require(!gateway.dispatchOrder(*normalized, allowed).dispatched,
            "duplicate local and idempotency identity must fail");
}

void testGatewayPartialFillAndExplicitCancel()
{
    SystemConfig config;
    config.mode = SystemMode::PAPER;
    PortfolioManager portfolio;
    BrokerGateway gateway(config, portfolio);
    gateway.setInstrumentSpec(makeInstrumentSpec());
    gateway.connect();
    gateway.injectNextPartialFill(0.5);

    auto request = makeRequest(11, 100);
    request.referencePrice = Decimal64{10000, 2};
    const auto normalized = gateway.normalizeOrder(request);
    require(normalized.has_value(), "partial-fill order should normalize");
    RiskDecision allowed;
    allowed.allowed = true;
    allowed.action = RuleBreachAction::Warn;
    require(gateway.dispatchOrder(*normalized, allowed).dispatched,
            "partial-fill order should dispatch");
    auto lifecycle = gateway.orderLifecycle(11);
    require(lifecycle.has_value()
                && lifecycle->state == OrderLifecycleState::PartiallyFilled,
            "partial fill must remain non-terminal");

    CancelRequest cancel;
    cancel.localOrderId = 11;
    cancel.externalOrderId = lifecycle->externalOrderId;
    cancel.timestampNs = 2'000;
    cancel.sequence = lifecycle->lastSequence + 1;
    cancel.idempotencyKey = "cancel-11";
    require(gateway.requestCancel(cancel),
            "cancel dispatch should be explicit and accepted");
    lifecycle = gateway.orderLifecycle(11);
    require(lifecycle.has_value()
                && lifecycle->state == OrderLifecycleState::Canceled,
            "cancel acknowledgement must transition to canceled");
}

void testModeBoundariesFailClosed()
{
    PortfolioManager portfolio;

    SystemConfig backtest;
    backtest.mode = SystemMode::BACKTEST;
    BrokerGateway backtestGateway(backtest, portfolio);
    backtestGateway.connect();
    require(!backtestGateway.isConnected(),
            "BACKTEST gateway must remain offline");

    SystemConfig live;
    live.mode = SystemMode::LIVE;
    BrokerGateway liveGateway(live, portfolio);
    liveGateway.connect();
    require(!liveGateway.isConnected(),
            "LIVE without a separately approved adapter must fail closed");
    require(liveGateway.adapterHealth().failure == FailureCategory::Validation,
            "fail-closed LIVE boundary must expose a validation failure");
}

} // namespace

int main()
{
    testDecimal64();
    testAcknowledgementDoesNotFill();
    testPartialAndFinalFillDeduplication();
    testCancelUnknownRequiresReconciliation();
    testInvalidAndStaleEvents();
    testGatewayNormalizationAndLifecycleEvents();
    testGatewayPartialFillAndExplicitCancel();
    testModeBoundariesFailClosed();
    std::cout << "phase22_tests: PASS\n";
    return 0;
}
