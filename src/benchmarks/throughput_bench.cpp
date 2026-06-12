#include "AnalyticsEngine.hpp"
#include "BrokerGateway.hpp"
#include "ExecutionEngine.hpp"
#include "LiveDataAdapter.hpp"
#include "MetricsAggregator.hpp"
#include "PortfolioManager.hpp"
#include "RiskEngine.hpp"
#include "SystemConfig.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
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
    const long long parsed = std::atoll(argv[1]);
    if (parsed <= 0) {
        return 1'000'000;
    }
    return static_cast<std::size_t>(parsed);
}

} // namespace

int main(int argc, char* argv[])
{
    const std::size_t tickCount = parseTickCount(argc, argv);
    std::cout << "[throughput_bench] Starting with tickCount=" << tickCount << "\n";

    SystemConfig cfg;
    cfg.mode = SystemMode::PAPER;
    cfg.reconnectInitialMs = 100;
    cfg.reconnectBackoffFactor = 2.0;
    cfg.reconnectMaxMs = 2000;

    PortfolioManager portfolio;
    RiskEngine       riskEngine(portfolio, 0, 0.0);
    riskEngine.setSystemConfig(&cfg);
    AnalyticsEngine  analytics;
    LiveDataAdapter  liveAdapter(cfg);
    BrokerGateway    brokerGateway(cfg, portfolio);
    ExecutionEngine  executionEngine(portfolio, riskEngine, "BENCH", 0.001, 5.0, 0.01);
    executionEngine.setAnalyticsEngine(&analytics);
    executionEngine.bindBrokerGateway(&brokerGateway);

    liveAdapter.connect();
    brokerGateway.connect();

    std::vector<uint64_t> ingestNs(tickCount + 1, 0);

    MetricsAggregator aggregator(tickCount);
    std::size_t fillCount = 0;

    brokerGateway.addFillCallback([&](const BrokerFill& fill) {
        if (!fill.success) { return; }
        if (fill.fillTimestamp == 0 || fill.fillTimestamp > tickCount) { return; }

        const auto idx = static_cast<std::size_t>(fill.fillTimestamp);
        const uint64_t t0 = ingestNs[idx];
        if (t0 == 0) { return; }

        const uint64_t dtNs = nowNs() - t0;
        if (aggregator.recordLatencyNs(dtNs)) {
            ++fillCount;
        }
    });

    // Suppress noisy per-trade logs during the sustained stress run.
    std::ostringstream muted;
    std::streambuf* oldCoutBuf = std::cout.rdbuf();
    std::cout.rdbuf(muted.rdbuf());

    std::size_t produced = 0;
    std::size_t consumed = 0;

    const auto tStart = std::chrono::steady_clock::now();

    bool longOpen = false;
    for (std::size_t i = 1; i <= tickCount; ++i) {
        const double base = 100.0 + static_cast<double>(i % 250) * 0.01;

        MarketCandle c;
        c.date = "2026.03.25";
        c.time = "00:00";
        c.symbol = "BENCH";
        c.epochTimestamp = static_cast<uint64_t>(i);
        c.open = base;
        c.high = base + 0.15;
        c.low = base - 0.15;
        c.close = base + std::sin(static_cast<double>(i) * 0.0001) * 0.05;
        c.volume = 1.0 + static_cast<double>(i % 10);

        ingestNs[i] = nowNs();
        liveAdapter.pushSimulatedCandle(c);
        ++produced;

        const auto next = liveAdapter.getNextCandle(0);
        if (!next.has_value()) {
            continue;
        }
        const Signal s = longOpen ? Signal::SELL : Signal::BUY;
        longOpen = !longOpen;
        executionEngine.execute(s, next->close, next->epochTimestamp, "BENCH");
        ++consumed;
    }

    const auto tEnd = std::chrono::steady_clock::now();
    std::cout.rdbuf(oldCoutBuf);

    const double durationMs = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(tEnd - tStart).count());
    const double throughput = (durationMs > 0.0)
        ? (static_cast<double>(consumed) * 1000.0 / durationMs)
        : 0.0;

    const auto summary = aggregator.summarize();
    const bool csvOk = aggregator.exportCsv("data/results/latency_report.csv",
                                            summary, durationMs, throughput);

    const double p99Ms = summary.p99Us / 1000.0;

    std::cout << "[throughput_bench] produced=" << produced
              << " consumed=" << consumed
              << " fills=" << fillCount << "\n";
    std::cout << "[throughput_bench] duration_ms=" << durationMs
              << " throughput_msg_per_sec=" << throughput << "\n";
    std::cout << "[throughput_bench] p50_us=" << summary.p50Us
              << " p95_us=" << summary.p95Us
              << " p99_us=" << summary.p99Us
              << " max_us=" << summary.maxUs << "\n";
    std::cout << "[throughput_bench] latency_report.csv=" << (csvOk ? "OK" : "FAILED") << "\n";

    if (!csvOk) {
        std::cerr << "[throughput_bench] ERROR: failed to export latency report.\n";
        return EXIT_FAILURE;
    }
    if (produced < tickCount || consumed < tickCount || fillCount < consumed) {
        std::cerr << "[throughput_bench] ERROR: did not process all ticks.\n";
        return EXIT_FAILURE;
    }
    if (throughput < 10'000.0) {
        std::cerr << "[throughput_bench] ERROR: throughput below 10,000 msg/s (" << throughput << ").\n";
        return EXIT_FAILURE;
    }
    if (p99Ms >= 5.0) {
        std::cerr << "[throughput_bench] ERROR: p99 latency breached 5ms envelope (" << p99Ms << "ms).\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
