#include "L2OrderBook.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {

struct BboUpdate {
    double bidPrice{0.0};
    double bidQty{0.0};
    double askPrice{0.0};
    double askQty{0.0};
};

std::size_t parseUpdates(int argc, char* argv[], bool& ok)
{
    ok = true;
    if (argc < 2) {
        return 100000;
    }
    if (argc > 2) {
        ok = false;
        return 0;
    }

    errno = 0;
    char* end = nullptr;
    const unsigned long long parsed = std::strtoull(argv[1], &end, 10);
    if (errno != 0 || end == argv[1] || *end != '\0' || parsed == 0 ||
        parsed > static_cast<unsigned long long>(std::numeric_limits<std::size_t>::max())) {
        ok = false;
        return 0;
    }
    return static_cast<std::size_t>(parsed);
}

std::vector<BboUpdate> generateUpdates(std::size_t updates)
{
    std::vector<BboUpdate> out;
    out.reserve(updates);

    constexpr double base = 63000.00;
    for (std::size_t i = 0; i < updates; ++i) {
        const int saw = static_cast<int>(i % 2000) - 1000;
        const double mid = base + static_cast<double>(saw) * 0.01;
        const double spread = 0.02 + static_cast<double>(i % 5) * 0.01;

        BboUpdate u;
        u.bidPrice = mid - spread * 0.5;
        u.askPrice = mid + spread * 0.5;
        u.bidQty = 1.0 + static_cast<double>(i % 7) * 0.125;
        u.askQty = 1.0 + static_cast<double>(i % 11) * 0.125;
        out.push_back(u);
    }

    return out;
}

double us(uint64_t ns) noexcept
{
    return static_cast<double>(ns) / 1000.0;
}

double percentileUs(const std::vector<uint64_t>& sorted, double pct) noexcept
{
    if (sorted.empty()) {
        return 0.0;
    }
    const double rank = (pct / 100.0) * static_cast<double>(sorted.size() - 1);
    const std::size_t idx = static_cast<std::size_t>(rank);
    return us(sorted[idx]);
}

bool finitePositive(double v) noexcept
{
    return std::isfinite(v) && v > 0.0;
}

} // namespace

int main(int argc, char* argv[])
{
    bool parseOk = false;
    const std::size_t updates = parseUpdates(argc, argv, parseOk);
    if (!parseOk) {
        std::cerr << "[apply_bbo_microbench] invalid update count\n";
        return EXIT_FAILURE;
    }

    const std::vector<BboUpdate> bboUpdates = generateUpdates(updates);
    std::vector<uint64_t> samplesNs;
    samplesNs.reserve(updates);

    L2OrderBook book(0.01, 262144);

    std::size_t applied = 0;
    const auto benchStart = std::chrono::steady_clock::now();
    for (const auto& update : bboUpdates) {
        const auto callStart = std::chrono::steady_clock::now();
        const bool ok = book.applyBbo(update.bidPrice, update.bidQty,
                                      update.askPrice, update.askQty);
        const auto callEnd = std::chrono::steady_clock::now();
        if (!ok) {
            std::cerr << "[apply_bbo_microbench] applyBbo failed at update=" << applied << "\n";
            return EXIT_FAILURE;
        }
        samplesNs.push_back(static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(callEnd - callStart).count()));
        ++applied;
    }
    const auto benchEnd = std::chrono::steady_clock::now();

    std::sort(samplesNs.begin(), samplesNs.end());

    uint64_t sumNs = 0;
    for (const uint64_t sample : samplesNs) {
        sumNs += sample;
    }

    const double elapsedMs = static_cast<double>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(benchEnd - benchStart).count()) /
        1'000'000.0;
    const double throughput = (elapsedMs > 0.0)
        ? static_cast<double>(updates) * 1000.0 / elapsedMs
        : 0.0;
    const double avgUs = us(sumNs) / static_cast<double>(samplesNs.size());
    const double p50Us = percentileUs(samplesNs, 50.0);
    const double p95Us = percentileUs(samplesNs, 95.0);
    const double p99Us = percentileUs(samplesNs, 99.0);
    const double maxUs = us(samplesNs.back());

    if (!finitePositive(elapsedMs) || !finitePositive(throughput) ||
        !finitePositive(avgUs) || !finitePositive(p50Us) ||
        p95Us < p50Us || p99Us < p95Us || maxUs < p99Us ||
        !std::isfinite(p95Us) || !std::isfinite(p99Us) || !std::isfinite(maxUs)) {
        std::cerr << "[apply_bbo_microbench] invalid metric output\n";
        return EXIT_FAILURE;
    }

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "[apply_bbo_microbench] updates=" << updates << "\n";
    std::cout << "[apply_bbo_microbench] elapsed_ms=" << elapsedMs << "\n";
    std::cout << "[apply_bbo_microbench] throughput_msg_per_sec=" << throughput << "\n";
    std::cout << "[apply_bbo_microbench] avg_us=" << avgUs << "\n";
    std::cout << "[apply_bbo_microbench] p50_us=" << p50Us << "\n";
    std::cout << "[apply_bbo_microbench] p95_us=" << p95Us << "\n";
    std::cout << "[apply_bbo_microbench] p99_us=" << p99Us << "\n";
    std::cout << "[apply_bbo_microbench] max_us=" << maxUs << "\n";

    return EXIT_SUCCESS;
}
