#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// Broker-neutral values and events used at the BrokerGateway boundary.
// Provider-native schemas must be translated before reaching these types.

enum class DecimalRounding : std::uint8_t {
    TowardZero,
    RejectUnaligned,
    NearestTiesAwayFromZero
};

struct Decimal64 {
    std::int64_t units{0};
    std::uint8_t scale{0};

    static constexpr std::uint8_t MAX_SCALE = 12;

    static std::optional<Decimal64> fromDouble(
        double value,
        std::uint8_t scale,
        DecimalRounding rounding = DecimalRounding::NearestTiesAwayFromZero) noexcept
    {
        if (!std::isfinite(value) || scale > MAX_SCALE) {
            return std::nullopt;
        }

        std::int64_t factor = 1;
        for (std::uint8_t i = 0; i < scale; ++i) {
            if (factor > std::numeric_limits<std::int64_t>::max() / 10) {
                return std::nullopt;
            }
            factor *= 10;
        }

        const long double scaled = static_cast<long double>(value)
                                 * static_cast<long double>(factor);
        const long double minValue = static_cast<long double>(
            std::numeric_limits<std::int64_t>::min());
        const long double maxValue = static_cast<long double>(
            std::numeric_limits<std::int64_t>::max());
        if (scaled < minValue || scaled > maxValue) {
            return std::nullopt;
        }

        long double rounded = 0.0L;
        switch (rounding) {
            case DecimalRounding::TowardZero:
                rounded = std::trunc(scaled);
                break;
            case DecimalRounding::RejectUnaligned:
                rounded = std::round(scaled);
                if (std::fabs(scaled - rounded) > 1e-9L) {
                    return std::nullopt;
                }
                break;
            case DecimalRounding::NearestTiesAwayFromZero:
                rounded = std::round(scaled);
                break;
        }
        return Decimal64{static_cast<std::int64_t>(rounded), scale};
    }

    double toDouble() const noexcept
    {
        double factor = 1.0;
        for (std::uint8_t i = 0; i < scale; ++i) {
            factor *= 10.0;
        }
        return static_cast<double>(units) / factor;
    }

    bool isPositive() const noexcept { return units > 0; }
    bool isNegative() const noexcept { return units < 0; }
    bool isZero() const noexcept { return units == 0; }

    friend bool operator==(const Decimal64&, const Decimal64&) = default;
};

enum class OrderSide : std::uint8_t { Buy, Sell };
enum class BrokerOrderType : std::uint8_t { Market, Limit, Stop, Unknown };

enum class OrderLifecycleState : std::uint8_t {
    Created,
    RiskChecked,
    Submitted,
    Accepted,
    PartiallyFilled,
    Filled,
    CancelRequested,
    Canceled,
    Expired,
    Rejected,
    Timeout,
    Unknown,
    Reconciled
};

enum class FailureCategory : std::uint8_t {
    None,
    Validation,
    Rejected,
    Transport,
    Timeout,
    Authentication,
    RateLimited,
    StaleData,
    MalformedEvent,
    ReconciliationMismatch,
    Unknown
};

enum class AdapterHealthState : std::uint8_t {
    Disconnected,
    Connected,
    Degraded,
    Stale,
    RateLimited,
    AuthenticationFailed,
    Unknown
};

enum class CancelDisposition : std::uint8_t {
    Accepted,
    Rejected,
    AlreadyFilled,
    AlreadyCanceled,
    Unknown
};

enum class ReconciliationStatus : std::uint8_t {
    Matched,
    LocalOnly,
    ExternalOnly,
    QuantityMismatch,
    PriceOrCostMismatch,
    IdentityMismatch,
    Stale,
    Unsupported,
    Unknown
};

enum class RuleBreachAction : std::uint8_t {
    Warn,
    Pause,
    Flatten,
    CancelPending,
    CloseOnly,
    ReadOnly,
    Halt,
    Termination,
    RewardAdjustment
};

struct OrderRequest {
    std::uint32_t schemaVersion{1};
    std::uint64_t localOrderId{0};
    std::string canonicalSymbol;
    OrderSide side{OrderSide::Buy};
    BrokerOrderType type{BrokerOrderType::Market};
    Decimal64 quantity;
    std::optional<Decimal64> limitPrice;
    std::optional<Decimal64> stopPrice;
    std::optional<Decimal64> referencePrice;
    std::string sourceId;
    std::uint64_t timestampNs{0};
    std::uint64_t sequence{0};
    std::string idempotencyKey;
};

struct NormalizedOrder {
    OrderRequest request;
    std::string executionSymbol;
    Decimal64 normalizedQuantity;
    std::optional<Decimal64> normalizedLimitPrice;
    std::optional<Decimal64> normalizedStopPrice;
    std::optional<Decimal64> normalizedReferencePrice;
    std::uint64_t instrumentVersion{0};
};

struct OrderAcknowledgement {
    std::uint32_t schemaVersion{1};
    std::uint64_t localOrderId{0};
    std::string externalOrderId;
    bool accepted{false};
    FailureCategory failure{FailureCategory::None};
    std::string reason;
    std::uint64_t timestampNs{0};
    std::uint64_t sequence{0};
    std::string eventKey;
};

struct ExecutionEvent {
    std::uint32_t schemaVersion{1};
    std::uint64_t localOrderId{0};
    std::string externalOrderId;
    Decimal64 cumulativeFilledQuantity;
    Decimal64 remainingQuantity;
    Decimal64 fillPrice;
    Decimal64 fee;
    std::uint64_t timestampNs{0};
    std::uint64_t sequence{0};
    std::string eventKey;
};

struct CancelRequest {
    std::uint32_t schemaVersion{1};
    std::uint64_t localOrderId{0};
    std::string externalOrderId;
    std::uint64_t timestampNs{0};
    std::uint64_t sequence{0};
    std::string idempotencyKey;
};

struct CancelResult {
    std::uint32_t schemaVersion{1};
    std::uint64_t localOrderId{0};
    CancelDisposition disposition{CancelDisposition::Unknown};
    FailureCategory failure{FailureCategory::None};
    std::string reason;
    std::uint64_t timestampNs{0};
    std::uint64_t sequence{0};
    std::string eventKey;
};

struct PositionSnapshot {
    std::string canonicalSymbol;
    Decimal64 quantity;
    Decimal64 averagePrice;
    std::string logicalPositionId;
};

struct AccountSnapshot {
    std::uint32_t schemaVersion{1};
    std::uint64_t snapshotVersion{0};
    Decimal64 balance;
    Decimal64 equity;
    Decimal64 realizedPnl;
    Decimal64 unrealizedPnl;
    Decimal64 marginUsed;
    Decimal64 freeMargin;
    std::string currency;
    std::uint64_t sourceTimestampNs{0};
    std::uint64_t ingestionTimestampNs{0};
    bool complete{false};
    std::vector<std::string> missingFields;
};

struct InstrumentSpec {
    std::uint32_t schemaVersion{1};
    std::uint64_t version{0};
    std::string canonicalSymbol;
    std::string executionAlias;
    Decimal64 tickSize;
    Decimal64 contractSize;
    Decimal64 minimumQuantity;
    Decimal64 maximumQuantity;
    Decimal64 quantityStep;
    std::uint64_t effectiveTimestampNs{0};
    bool complete{false};
};

struct ReconciliationSnapshot {
    std::uint32_t schemaVersion{1};
    std::uint64_t snapshotVersion{0};
    std::uint64_t timestampNs{0};
    AccountSnapshot account;
    std::vector<PositionSnapshot> positions;
    std::unordered_map<std::uint64_t, OrderLifecycleState> orderStates;
    ReconciliationStatus status{ReconciliationStatus::Unknown};
};

struct AdapterHealthEvent {
    std::uint32_t schemaVersion{1};
    AdapterHealthState state{AdapterHealthState::Unknown};
    std::uint64_t timestampNs{0};
    std::uint64_t sequence{0};
    std::uint32_t latencyMs{0};
    FailureCategory failure{FailureCategory::None};
    std::string reason;
    std::string eventKey;
};

struct RuleLimit {
    std::string name;
    Decimal64 threshold;
    RuleBreachAction action{RuleBreachAction::Halt};
};

struct RuleProfile {
    std::uint32_t schemaVersion{1};
    std::string profileId;
    std::uint64_t version{0};
    std::string timezone{"UTC"};
    std::vector<RuleLimit> limits;
};

struct RiskDecision {
    bool allowed{false};
    bool riskIncreasing{true};
    FailureCategory failure{FailureCategory::None};
    RuleBreachAction action{RuleBreachAction::Halt};
    std::string reason;
};
