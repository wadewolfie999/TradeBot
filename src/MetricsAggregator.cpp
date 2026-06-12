#include "MetricsAggregator.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <numeric>

MetricsAggregator::MetricsAggregator(std::size_t capacity)
{
    m_samplesNs.resize(capacity, 0);
}

bool MetricsAggregator::recordLatencyNs(uint64_t latencyNs) noexcept
{
    if (m_size >= m_samplesNs.size()) {
        return false;
    }
    m_samplesNs[m_size++] = latencyNs;
    return true;
}

std::size_t MetricsAggregator::size() const noexcept
{
    return m_size;
}

std::size_t MetricsAggregator::capacity() const noexcept
{
    return m_samplesNs.size();
}

double MetricsAggregator::percentileUs(const std::vector<uint64_t>& sorted,
                                       double p) noexcept
{
    if (sorted.empty()) { return 0.0; }
    const double rank = (p / 100.0) * static_cast<double>(sorted.size() - 1);
    const std::size_t idx = static_cast<std::size_t>(rank);
    return static_cast<double>(sorted[idx]) / 1000.0;
}

MetricsAggregator::Summary MetricsAggregator::summarize() const
{
    Summary out;
    out.sampleCount = m_size;
    if (m_size == 0) {
        return out;
    }

    std::vector<uint64_t> sorted(m_samplesNs.begin(), m_samplesNs.begin() + static_cast<long>(m_size));
    std::sort(sorted.begin(), sorted.end());

    const uint64_t sumNs = std::accumulate(sorted.begin(), sorted.end(), uint64_t{0});
    out.meanUs = static_cast<double>(sumNs) / static_cast<double>(m_size) / 1000.0;
    out.maxUs  = static_cast<double>(sorted.back()) / 1000.0;
    out.p50Us  = percentileUs(sorted, 50.0);
    out.p90Us  = percentileUs(sorted, 90.0);
    out.p95Us  = percentileUs(sorted, 95.0);
    out.p99Us  = percentileUs(sorted, 99.0);
    return out;
}

bool MetricsAggregator::exportCsv(const std::string& path,
                                  const Summary&     summary,
                                  double             durationMs,
                                  double             throughputMsgPerSec) const
{
    const std::filesystem::path p(path);
    std::filesystem::create_directories(p.parent_path());

    std::ofstream ofs(path);
    if (!ofs.is_open()) { return false; }

    ofs << "metric,value,unit\n";
    ofs << std::fixed << std::setprecision(6);
    ofs << "sample_count," << summary.sampleCount << ",count\n";
    ofs << "duration_ms," << durationMs << ",ms\n";
    ofs << "throughput_msg_per_sec," << throughputMsgPerSec << ",msg/s\n";
    ofs << "mean_latency_us," << summary.meanUs << ",us\n";
    ofs << "max_latency_us," << summary.maxUs << ",us\n";
    ofs << "p50_latency_us," << summary.p50Us << ",us\n";
    ofs << "p90_latency_us," << summary.p90Us << ",us\n";
    ofs << "p95_latency_us," << summary.p95Us << ",us\n";
    ofs << "p99_latency_us," << summary.p99Us << ",us\n";
    return true;
}
