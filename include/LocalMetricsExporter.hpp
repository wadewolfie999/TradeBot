#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

class LocalMetricsExporter {
public:
    LocalMetricsExporter() = default;
    ~LocalMetricsExporter();

    bool start(uint16_t port = 9464);
    void stop() noexcept;
    bool isRunning() const noexcept;
    uint16_t port() const noexcept;

    void setGauge(const std::string& name, double value);
    void incrementCounter(const std::string& name, double delta = 1.0);
    double gauge(const std::string& name) const;
    double counter(const std::string& name) const;

    void ingestSystemSnapshot(double networkHandoffP99Ms,
                              std::size_t queueDepth,
                              uint64_t ordersSubmitted,
                              uint64_t fillsReceived,
                              uint64_t authFailures,
                              uint64_t connectEvents,
                              uint64_t disconnectEvents,
                              uint64_t reconnectEvents);

    std::string renderPrometheusText() const;
    uint64_t totalScrapes() const noexcept;
    std::string lastError() const;

private:
    static std::string sanitizeMetricName(std::string name);
    void runServerLoop();
    void setLastError(const std::string& msg);

    mutable std::mutex m_metricsMutex;
    std::unordered_map<std::string, double> m_gauges;
    std::unordered_map<std::string, double> m_counters;

    std::atomic<bool> m_running{false};
    std::thread m_serverThread;
    int m_listenFd{-1};
    uint16_t m_port{0};
    std::atomic<uint64_t> m_totalScrapes{0};

    mutable std::mutex m_errorMutex;
    std::string m_lastError;
};
