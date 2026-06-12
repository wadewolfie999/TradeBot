#include "AnalyticsEngine.hpp"
#include "BrokerGateway.hpp"
#include "ExecutionEngine.hpp"
#include "LiveDataAdapter.hpp"
#include "LockFreeRingBuffer.hpp"
#include "PortfolioManager.hpp"
#include "RiskEngine.hpp"
#include "SystemConfig.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {

void require(bool cond, const char* msg)
{
    if (!cond) {
        throw std::runtime_error(msg);
    }
}

void test_lock_free_queue_concurrency()
{
    LockFreeRingBuffer<uint64_t, 2048> queue;
    constexpr uint64_t N = 200'000;
    std::atomic<bool> producerDone{false};
    std::atomic<bool> orderValid{true};

    std::thread producer([&] {
        for (uint64_t i = 1; i <= N; ++i) {
            while (!queue.push(i)) {
                std::this_thread::yield();
            }
        }
        producerDone.store(true);
    });

    std::thread consumer([&] {
        uint64_t expected = 1;
        uint64_t value = 0;
        while (!producerDone.load() || expected <= N) {
            if (!queue.pop(value)) {
                std::this_thread::yield();
                continue;
            }
            if (value != expected) {
                orderValid.store(false);
                return;
            }
            ++expected;
        }
    });

    producer.join();
    consumer.join();
    require(orderValid.load(), "Lock-free queue violated FIFO ordering under concurrency");
}

void test_partial_fill_resolution()
{
    SystemConfig cfg;
    cfg.mode = SystemMode::PAPER;

    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 3);
    risk.setSystemConfig(&cfg);
    AnalyticsEngine analytics;
    BrokerGateway broker(cfg, portfolio);
    broker.connect();

    ExecutionEngine engine(portfolio, risk, "BTCUSDT", 0.001, 5.0, 0.01);
    engine.setAnalyticsEngine(&analytics);
    engine.bindBrokerGateway(&broker);

    broker.injectNextPartialFill(0.4);
    const bool submitted = engine.execute(Signal::BUY, 100.0, 1'000, "SMA_01");
    require(submitted, "Initial buy order was not submitted");

    const uint64_t orderId = engine.lastBrokerOrderId();
    require(orderId > 0, "Broker order ID was not captured");
    const double firstRemaining = engine.pendingBrokerQuantity(orderId);
    require(firstRemaining > 0.0, "Partial fill did not leave remaining quantity");
    const double firstQty = portfolio.getPositionQuantity("BTCUSDT");
    require(firstQty > 0.0, "No position was opened after partial buy fill");

    BrokerFill secondFill;
    secondFill.orderId = orderId;
    secondFill.symbol = "BTCUSDT";
    secondFill.isBuy = true;
    secondFill.requestedPrice = 100.0;
    secondFill.fillPrice = 100.05;
    secondFill.quantity = firstRemaining;
    secondFill.filledQuantity = firstRemaining;
    secondFill.remainingQuantity = 0.0;
    secondFill.partialFill = false;
    secondFill.feePaid = secondFill.fillPrice * secondFill.filledQuantity * 0.001;
    secondFill.fillTimestamp = 1'001;
    secondFill.success = true;
    broker.simulateFillConfirmation(secondFill);

    const double secondQty = portfolio.getPositionQuantity("BTCUSDT");
    require(secondQty > firstQty, "Second fill did not increase position quantity");
    require(engine.pendingBrokerQuantity(orderId) == 0.0,
            "Pending quantity was not cleared after completion fill");
    require(std::abs(risk.syncedPosition("BTCUSDT") - secondQty) < 1e-9,
            "RiskEngine position sync mismatch after partial fill resolution");
}

void test_latency_assertions()
{
    SystemConfig cfg;
    cfg.mode = SystemMode::PAPER;

    PortfolioManager portfolio;
    RiskEngine risk(portfolio, 3);
    risk.setSystemConfig(&cfg);
    AnalyticsEngine analytics;
    BrokerGateway broker(cfg, portfolio);
    broker.connect();

    ExecutionEngine engine(portfolio, risk, "BTCUSDT", 0.001, 5.0, 0.01);
    engine.setAnalyticsEngine(&analytics);
    engine.bindBrokerGateway(&broker);

    for (uint64_t i = 0; i < 200; ++i) {
        engine.execute(Signal::BUY, 100.0 + static_cast<double>(i) * 0.01, 2'000 + i, "SMA_01");
        engine.execute(Signal::SELL, 100.2 + static_cast<double>(i) * 0.01, 2'100 + i, "SMA_01");
    }

    const double maxLatencyMs = engine.getMaxRouteLatencyMs();
    require(maxLatencyMs < 5.0, "Order route latency exceeded 5ms envelope");
    require(engine.getLatencyBreachCount() == 0, "Latency breach counter non-zero");
}

void test_reconnect_recovery_under_2s()
{
    SystemConfig cfg;
    cfg.mode = SystemMode::PAPER;
    cfg.reconnectInitialMs = 100;
    cfg.reconnectBackoffFactor = 2.0;
    cfg.reconnectMaxMs = 1000;

    LiveDataAdapter adapter(cfg);
    std::atomic<bool> degraded{false};
    std::atomic<bool> restored{false};
    std::chrono::steady_clock::time_point tDisconnect{};
    std::chrono::steady_clock::time_point tRestore{};

    adapter.setIntegrityCallback([&](bool integrityOk) {
        if (!integrityOk) {
            degraded.store(true);
            tDisconnect = std::chrono::steady_clock::now();
        } else if (degraded.load()) {
            restored.store(true);
            tRestore = std::chrono::steady_clock::now();
        }
    });

    adapter.connect();
    adapter.simulateDisconnect();

    const auto start = std::chrono::steady_clock::now();
    while (!adapter.isConnected()) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(2)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    require(adapter.isConnected(), "LiveDataAdapter did not reconnect");
    require(restored.load(), "Integrity restoration callback not fired");
    const auto reconnectMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        tRestore - tDisconnect).count();
    require(reconnectMs < 2000, "Reconnect time exceeded 2000ms");
    require(adapter.gapFillCount() > 0, "Gap-fill was not triggered after reconnect");
}

} // namespace

int main()
{
    test_lock_free_queue_concurrency();
    test_partial_fill_resolution();
    test_latency_assertions();
    test_reconnect_recovery_under_2s();
    std::cout << "[phase13_tests] All tests passed.\n";
    return 0;
}
