#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class MetricsAggregator {
public:
    struct Summary {
        std::size_t sampleCount{0};
        double meanUs{0.0};
        double maxUs{0.0};
        double p50Us{0.0};
        double p90Us{0.0};
        double p95Us{0.0};
        double p99Us{0.0};
    };

    explicit MetricsAggregator(std::size_t capacity);

    // Critical-path safe: writes into pre-allocated storage only.
    bool recordLatencyNs(uint64_t latencyNs) noexcept;

    std::size_t size() const noexcept;
    std::size_t capacity() const noexcept;

    Summary summarize() const;

    bool exportCsv(const std::string& path,
                   const Summary&     summary,
                   double             durationMs,
                   double             throughputMsgPerSec) const;

private:
    static double percentileUs(const std::vector<uint64_t>& sorted,
                               double p) noexcept;

    std::vector<uint64_t> m_samplesNs;
    std::size_t           m_size{0};
};
