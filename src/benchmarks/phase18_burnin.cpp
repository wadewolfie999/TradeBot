#include "ExecutionEngine.hpp"
#include "L2OrderBook.hpp"
#include "LocalDataReplayAdapter.hpp"
#include "LocalMetricsExporter.hpp"
#include "MetricsAggregator.hpp"
#include "PortfolioManager.hpp"
#include "RiskEngine.hpp"
#include "TriggerOrderManager.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

uint64_t nowNs() noexcept
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

std::size_t parseTickCount(int argc, char* argv[]) noexcept
{
    if (argc < 2) {
        return 1'000'000;
    }
    const long long n = std::atoll(argv[1]);
    if (n <= 0) {
        return 1'000'000;
    }
    return static_cast<std::size_t>(n);
}

std::string parseReplayPath(int argc, char* argv[])
{
    if (argc < 3) {
        return "data/historical/BTCUSDT-L2-1M.bin";
    }
    return argv[2];
}

bool ensureReplayDataset(const std::string& path, std::size_t tickCount)
{
    if (std::filesystem::exists(path)) {
        return true;
    }

    std::vector<ReplayTick> ticks;
    ticks.reserve(tickCount);

    double px = 63000.0;
    uint64_t ts = 1'710'000'000'000'000'000ULL;

    for (std::size_t i = 0; i < tickCount; ++i) {
        const double drift = std::sin(static_cast<double>(i) * 0.00035) * 0.9;
        const double pulse = ((i % 64) < 32) ? 0.015 : -0.015;
        px += drift + pulse;

        const double spread = 0.60 + std::fmod(static_cast<double>(i), 17.0) * 0.01;
        ReplayTick tick;
        tick.timestampNs = ts;
        tick.symbol = "BTCUSDT";
        tick.bidPrice = px - spread * 0.5;
        tick.askPrice = px + spread * 0.5;
        tick.bidSize = 1.0 + static_cast<double>(i % 7) * 0.2;
        tick.askSize = 1.0 + static_cast<double>(i % 9) * 0.2;
        tick.tradePrice = px + std::sin(static_cast<double>(i) * 0.002) * 0.05;
        tick.tradeSize = 0.2 + static_cast<double>(i % 11) * 0.03;
        tick.flags = 1u;

        ticks.push_back(std::move(tick));
        ts += 1'000'000; // 1ms synthetic event spacing
    }

    std::string err;
    if (!LocalDataReplayAdapter::writeBinary(path, ticks, &err)) {
        std::cerr << "[phase18_burnin] dataset write failed: " << err << "\n";
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char* argv[])
{
    const std::size_t tickCount = parseTickCount(argc, argv);
    const std::string replayPath = parseReplayPath(argc, argv);

    if (!ensureReplayDataset(replayPath, tickCount)) {
        return 1;
    }

    LocalDataReplayAdapter replay;
    if (!replay.loadFromPath(replayPath, "BTCUSDT")) {
        std::cerr << "[phase18_burnin] replay load failed: " << replay.lastError() << "\n";
        return 1;
    }
    replay.setSpeedMode(ReplaySpeedMode::MAX_EFFORT);

    PortfolioManager portfolio;
    RiskEngine riskEngine(portfolio, 1, 0.0);
    ExecutionEngine execution(portfolio, riskEngine, "BTCUSDT", 0.001, 5.0, 0.01);

    TriggerOrderManager triggerOrders(8192);
    execution.bindTriggerOrderManager(&triggerOrders);

    L2OrderBook orderBook(0.01, 262144);
    MetricsAggregator aggregator(tickCount);
    LocalMetricsExporter metrics;

    std::size_t consumed = 0;
    std::size_t ocoPlaced = 0;
    std::size_t stopPlaced = 0;
    std::size_t triggerFills = 0;
    bool protectiveOrdersArmed = false;

    ReplayTick tick;
    const auto tStart = std::chrono::steady_clock::now();

    std::ostringstream muted;
    std::streambuf* oldCoutBuf = std::cout.rdbuf();
    std::cout.rdbuf(muted.rdbuf());

    while (consumed < tickCount && replay.nextTick(tick)) {
        const uint64_t t0 = nowNs();

        if (!orderBook.applyBbo(tick.bidPrice, tick.bidSize, tick.askPrice, tick.askSize)) {
            continue;
        }

        if (!portfolio.hasPosition("BTCUSDT")) {
            (void)execution.execute(Signal::BUY, tick.askPrice, tick.timestampNs, "PHASE18_BURNIN");
            protectiveOrdersArmed = false;
        }

        if (portfolio.hasPosition("BTCUSDT") && !protectiveOrdersArmed) {
            const double entry = portfolio.getEntryPrice("BTCUSDT");
            const uint64_t ocoId = execution.placeOcoTrigger(entry * 0.9992,
                                                              entry * 1.0008,
                                                              0.0,
                                                              "PHASE18_OCO");
            if (ocoId != 0) {
                ++ocoPlaced;
                protectiveOrdersArmed = true;
            }

            if ((consumed % 50000) == 0) {
                const uint64_t stopId = execution.placeStopLossTrigger(entry * 0.9988,
                                                                       0.0,
                                                                       "PHASE18_STOP");
                if (stopId != 0) {
                    ++stopPlaced;
                }
            }
        }

        triggerFills += static_cast<std::size_t>(
            execution.processTriggerOrders(orderBook, tick.timestampNs));

        portfolio.updatePnL("BTCUSDT", tick.tradePrice);
        riskEngine.pushEquityReturn(portfolio.getTotalEquity());

        const uint64_t dtNs = nowNs() - t0;
        (void)aggregator.recordLatencyNs(dtNs);

        ++consumed;
    }

    std::cout.rdbuf(oldCoutBuf);

    const auto tEnd = std::chrono::steady_clock::now();
    const double durationMs = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count());
    const double throughput = (durationMs > 0.0)
        ? (static_cast<double>(consumed) * 1000.0 / durationMs)
        : 0.0;

    const auto summary = aggregator.summarize();
    const double p99Ms = summary.p99Us / 1000.0;

    metrics.ingestSystemSnapshot(
        p99Ms,
        0,
        static_cast<uint64_t>(execution.getSignalCount()),
        static_cast<uint64_t>(execution.getFilledCount()),
        0,
        0,
        0,
        0);

    const bool csvOk = aggregator.exportCsv("data/results/phase18_burnin_latency.csv",
                                            summary,
                                            durationMs,
                                            throughput);

    std::cout << "[phase18_burnin] ticks=" << consumed
              << " ocoPlaced=" << ocoPlaced
              << " stopPlaced=" << stopPlaced
              << " triggerFills=" << triggerFills << "\n";
    std::cout << "[phase18_burnin] duration_ms=" << durationMs
              << " throughput_msg_per_sec=" << throughput << "\n";
    std::cout << "[phase18_burnin] p50_us=" << summary.p50Us
              << " p95_us=" << summary.p95Us
              << " p99_us=" << summary.p99Us
              << " max_us=" << summary.maxUs << "\n";
    std::cout << "[phase18_burnin] exporter_p99_gauge="
              << metrics.gauge("aiio_network_handoff_p99_ms")
              << "\n";

    if (!csvOk) {
        std::cerr << "[phase18_burnin] ERROR: failed to export latency CSV\n";
        return 1;
    }
    if (consumed < tickCount) {
        std::cerr << "[phase18_burnin] ERROR: replay exhausted early\n";
        return 1;
    }
    if (p99Ms >= 5.0) {
        std::cerr << "[phase18_burnin] ERROR: p99 latency breached 5ms envelope ("
                  << p99Ms << "ms)\n";
        return 1;
    }

    return 0;
}
