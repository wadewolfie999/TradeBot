#include "DeterministicBrokerAdapter.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {

Decimal64 multiplyDecimal(Decimal64 lhs, Decimal64 rhs) noexcept
{
    const long double value = static_cast<long double>(lhs.toDouble())
                            * static_cast<long double>(rhs.toDouble());
    const std::uint8_t scale = std::min<std::uint8_t>(
        Decimal64::MAX_SCALE,
        static_cast<std::uint8_t>(lhs.scale + rhs.scale));
    return Decimal64::fromDouble(static_cast<double>(value), scale)
        .value_or(Decimal64{});
}

} // namespace

void DeterministicBrokerAdapter::setAcknowledgementCallback(
    AcknowledgementCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_acknowledgementCallback = std::move(callback);
}

void DeterministicBrokerAdapter::setExecutionCallback(ExecutionCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_executionCallback = std::move(callback);
}

void DeterministicBrokerAdapter::setCancelCallback(CancelCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cancelCallback = std::move(callback);
}

void DeterministicBrokerAdapter::setHealthCallback(HealthCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthCallback = std::move(callback);
}

bool DeterministicBrokerAdapter::connect()
{
    HealthCallback callback;
    AdapterHealthEvent event;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_connected) {
            return true;
        }
        if (m_everConnected) {
            ++m_reconnectCount;
        }
        m_connected = true;
        m_everConnected = true;
        ++m_connectCount;
        m_health = AdapterHealthEvent{
            1, AdapterHealthState::Connected, 0, ++m_sequence, 0,
            FailureCategory::None, {}, "health-" + std::to_string(m_sequence)};
        callback = m_healthCallback;
        event = m_health;
    }
    if (callback) {
        callback(event);
    }
    return true;
}

void DeterministicBrokerAdapter::disconnect() noexcept
{
    HealthCallback callback;
    AdapterHealthEvent event;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_connected) {
            return;
        }
        m_connected = false;
        ++m_disconnectCount;
        m_health = AdapterHealthEvent{
            1, AdapterHealthState::Disconnected, 0, ++m_sequence, 0,
            FailureCategory::None, {}, "health-" + std::to_string(m_sequence)};
        callback = m_healthCallback;
        event = m_health;
    }
    if (callback) {
        callback(event);
    }
}

bool DeterministicBrokerAdapter::isConnected() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connected;
}

bool DeterministicBrokerAdapter::submit(const NormalizedOrder& order)
{
    AcknowledgementCallback acknowledgementCallback;
    ExecutionCallback executionCallback;
    HealthCallback healthCallback;
    OrderAcknowledgement acknowledgement;
    ExecutionEvent execution;
    std::optional<AdapterHealthEvent> healthEvent;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_connected) {
            return false;
        }

        ++m_submitCount;
        const auto submitNumber = m_submitCount;
        const bool disconnect = m_faultConfig.enabled
            && m_faultConfig.disconnectEveryN > 0
            && submitNumber % m_faultConfig.disconnectEveryN == 0;
        const bool drop = m_faultConfig.enabled
            && m_faultConfig.dropEveryN > 0
            && submitNumber % m_faultConfig.dropEveryN == 0;
        const bool latency = m_faultConfig.enabled
            && m_faultConfig.latencySpikeEveryN > 0
            && m_faultConfig.latencySpikeMs > 0
            && submitNumber % m_faultConfig.latencySpikeEveryN == 0;

        if (disconnect) {
            ++m_faultDisconnects;
            ++m_disconnectCount;
            ++m_reconnectCount;
            m_connected = true;
            m_health = AdapterHealthEvent{
                1, AdapterHealthState::Connected, order.request.timestampNs,
                ++m_sequence, 0, FailureCategory::Transport,
                "deterministic disconnect and reconnect",
                "health-" + std::to_string(m_sequence)};
            healthEvent = m_health;
            healthCallback = m_healthCallback;
        }
        if (latency) {
            ++m_faultLatencySpikes;
            m_health.latencyMs = m_faultConfig.latencySpikeMs;
        }

        acknowledgement.schemaVersion = 1;
        acknowledgement.localOrderId = order.request.localOrderId;
        acknowledgement.timestampNs = order.request.timestampNs;
        acknowledgement.sequence = order.request.sequence + 3;
        acknowledgement.eventKey = "ack-" + std::to_string(order.request.localOrderId);

        const bool injectedError = m_injectError;
        if (injectedError || drop) {
            acknowledgement.accepted = false;
            acknowledgement.failure = drop
                ? FailureCategory::Transport : FailureCategory::Rejected;
            acknowledgement.reason = drop
                ? "deterministic fault drop" : m_injectedError;
            if (drop) {
                ++m_faultDrops;
            }
            m_injectError = false;
            m_injectedError.clear();
            acknowledgementCallback = m_acknowledgementCallback;
        } else {
            acknowledgement.accepted = true;
            acknowledgement.externalOrderId =
                "paper-" + std::to_string(order.request.localOrderId);
            acknowledgementCallback = m_acknowledgementCallback;

            Decimal64 filled = order.normalizedQuantity;
            Decimal64 remaining{0, order.normalizedQuantity.scale};
            if (m_partialFillRatio.has_value()) {
                const auto partial = Decimal64::fromDouble(
                    order.normalizedQuantity.toDouble() * *m_partialFillRatio,
                    order.normalizedQuantity.scale,
                    DecimalRounding::TowardZero);
                if (partial.has_value() && partial->isPositive()
                    && partial->units < order.normalizedQuantity.units) {
                    filled = *partial;
                    remaining.units = order.normalizedQuantity.units - filled.units;
                }
                m_partialFillRatio.reset();
            }

            execution.schemaVersion = 1;
            execution.localOrderId = order.request.localOrderId;
            execution.externalOrderId = acknowledgement.externalOrderId;
            execution.cumulativeFilledQuantity = filled;
            execution.remainingQuantity = remaining;
            execution.fillPrice = order.normalizedLimitPrice.value_or(
                order.normalizedStopPrice.value_or(
                    order.normalizedReferencePrice.value_or(Decimal64{})));
            if (order.request.type == BrokerOrderType::Market
                && execution.fillPrice.isPositive()) {
                const double slippage = order.request.side == OrderSide::Buy
                    ? 1.0005 : 0.9995;
                execution.fillPrice = Decimal64::fromDouble(
                    execution.fillPrice.toDouble() * slippage,
                    execution.fillPrice.scale,
                    DecimalRounding::NearestTiesAwayFromZero)
                    .value_or(Decimal64{});
            }
            execution.fee = multiplyDecimal(execution.fillPrice, filled);
            if (!execution.fee.isZero()) {
                execution.fee = Decimal64::fromDouble(
                    execution.fee.toDouble() * 0.001,
                    execution.fee.scale,
                    DecimalRounding::NearestTiesAwayFromZero).value_or(Decimal64{});
            }
            execution.timestampNs = order.request.timestampNs;
            execution.sequence = order.request.sequence + 4;
            execution.eventKey = "fill-" + std::to_string(order.request.localOrderId)
                               + "-1";
            executionCallback = m_executionCallback;
            if (!remaining.isZero()) {
                m_pendingOrders[order.request.localOrderId] = order;
            }
        }
    }

    if (healthEvent.has_value() && healthCallback) {
        healthCallback(*healthEvent);
    }
    if (acknowledgementCallback) {
        acknowledgementCallback(acknowledgement);
    }
    if (acknowledgement.accepted && executionCallback) {
        executionCallback(execution);
    }
    return true;
}

bool DeterministicBrokerAdapter::cancel(const CancelRequest& request)
{
    CancelCallback callback;
    CancelResult result;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_connected) {
            return false;
        }
        result.localOrderId = request.localOrderId;
        result.timestampNs = request.timestampNs;
        result.sequence = request.sequence + 1;
        result.eventKey = "cancel-result-" + std::to_string(request.localOrderId);
        const auto erased = m_pendingOrders.erase(request.localOrderId);
        result.disposition = erased > 0
            ? CancelDisposition::Accepted : CancelDisposition::AlreadyFilled;
        callback = m_cancelCallback;
    }
    if (callback) {
        callback(result);
    }
    return true;
}

ReconciliationSnapshot DeterministicBrokerAdapter::reconcile(
    std::uint64_t timestampNs)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    ReconciliationSnapshot snapshot;
    snapshot.snapshotVersion = ++m_sequence;
    snapshot.timestampNs = timestampNs;
    if (m_accountSnapshot.has_value()) {
        snapshot.account = *m_accountSnapshot;
    }
    for (const auto& [localOrderId, order] : m_pendingOrders) {
        static_cast<void>(order);
        snapshot.orderStates.emplace(localOrderId,
                                     OrderLifecycleState::PartiallyFilled);
    }
    snapshot.status = ReconciliationStatus::Matched;
    return snapshot;
}

AdapterHealthEvent DeterministicBrokerAdapter::health() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_health;
}

std::optional<AccountSnapshot> DeterministicBrokerAdapter::accountSnapshot() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_accountSnapshot;
}

std::optional<InstrumentSpec> DeterministicBrokerAdapter::instrumentSpec(
    const std::string& canonicalSymbol) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    const auto it = m_instruments.find(canonicalSymbol);
    return it == m_instruments.end()
        ? std::nullopt : std::optional<InstrumentSpec>(it->second);
}

void DeterministicBrokerAdapter::setInstrumentSpec(const InstrumentSpec& spec)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_instruments[spec.canonicalSymbol] = spec;
}

void DeterministicBrokerAdapter::setAccountSnapshot(const AccountSnapshot& snapshot)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_accountSnapshot = snapshot;
}

void DeterministicBrokerAdapter::injectNextError(std::string reason)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_injectError = true;
    m_injectedError = std::move(reason);
}

void DeterministicBrokerAdapter::injectNextPartialFill(double ratio) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (std::isfinite(ratio) && ratio > 0.0 && ratio < 1.0) {
        m_partialFillRatio = ratio;
    } else {
        m_partialFillRatio.reset();
    }
}

void DeterministicBrokerAdapter::setFaultConfig(const FaultConfig& config) noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_faultConfig = config;
}

DeterministicBrokerAdapter::FaultConfig
DeterministicBrokerAdapter::faultConfig() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_faultConfig;
}

std::uint64_t DeterministicBrokerAdapter::faultDropsTriggered() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_faultDrops;
}

std::uint64_t DeterministicBrokerAdapter::faultLatencySpikesTriggered() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_faultLatencySpikes;
}

std::uint64_t DeterministicBrokerAdapter::faultDisconnectsTriggered() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_faultDisconnects;
}

std::uint64_t DeterministicBrokerAdapter::connectCount() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connectCount;
}

std::uint64_t DeterministicBrokerAdapter::disconnectCount() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_disconnectCount;
}

std::uint64_t DeterministicBrokerAdapter::reconnectCount() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_reconnectCount;
}

void DeterministicBrokerAdapter::publishHealth(
    AdapterHealthState state,
    std::uint64_t timestampNs,
    std::uint32_t latencyMs,
    FailureCategory failure,
    const std::string& reason)
{
    HealthCallback callback;
    AdapterHealthEvent event;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_health = AdapterHealthEvent{
            1, state, timestampNs, ++m_sequence, latencyMs, failure, reason,
            "health-" + std::to_string(m_sequence)};
        callback = m_healthCallback;
        event = m_health;
    }
    if (callback) {
        callback(event);
    }
}
