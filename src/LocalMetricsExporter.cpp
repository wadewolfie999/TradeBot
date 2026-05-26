#include "LocalMetricsExporter.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstring>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

namespace {

int createLoopbackListener(uint16_t port, uint16_t& boundPort, std::string& err)
{
    const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        err = "metrics socket() failed";
        return -1;
    }

    int opt = 1;
    (void)::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = 200 * 1000;
    (void)::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    (void)::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(port);
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        err = "metrics bind() failed";
        ::close(fd);
        return -1;
    }
    if (::listen(fd, 16) != 0) {
        err = "metrics listen() failed";
        ::close(fd);
        return -1;
    }

    sockaddr_in bound{};
    socklen_t len = sizeof(bound);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&bound), &len) == 0) {
        boundPort = ntohs(bound.sin_port);
    } else {
        boundPort = port;
    }
    return fd;
}

} // namespace

LocalMetricsExporter::~LocalMetricsExporter()
{
    stop();
}

bool LocalMetricsExporter::start(uint16_t port)
{
    if (m_running.load()) {
        return true;
    }

    std::string err;
    uint16_t boundPort = 0;
    const int fd = createLoopbackListener(port, boundPort, err);
    if (fd < 0) {
        setLastError(err);
        return false;
    }

    m_listenFd = fd;
    m_port = boundPort;
    m_running.store(true);
    m_serverThread = std::thread([this] { runServerLoop(); });
    return true;
}

void LocalMetricsExporter::stop() noexcept
{
    if (!m_running.exchange(false)) {
        return;
    }

    if (m_listenFd >= 0) {
        ::close(m_listenFd);
        m_listenFd = -1;
    }
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
}

bool LocalMetricsExporter::isRunning() const noexcept
{
    return m_running.load();
}

uint16_t LocalMetricsExporter::port() const noexcept
{
    return m_port;
}

void LocalMetricsExporter::setGauge(const std::string& name, double value)
{
    const std::string metric = sanitizeMetricName(name);
    if (metric.empty()) { return; }
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_gauges[metric] = value;
}

void LocalMetricsExporter::incrementCounter(const std::string& name, double delta)
{
    const std::string metric = sanitizeMetricName(name);
    if (metric.empty()) { return; }
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    m_counters[metric] += delta;
}

double LocalMetricsExporter::gauge(const std::string& name) const
{
    const std::string metric = sanitizeMetricName(name);
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    const auto it = m_gauges.find(metric);
    return (it == m_gauges.end()) ? 0.0 : it->second;
}

double LocalMetricsExporter::counter(const std::string& name) const
{
    const std::string metric = sanitizeMetricName(name);
    std::lock_guard<std::mutex> lock(m_metricsMutex);
    const auto it = m_counters.find(metric);
    return (it == m_counters.end()) ? 0.0 : it->second;
}

void LocalMetricsExporter::ingestSystemSnapshot(double networkHandoffP99Ms,
                                                std::size_t queueDepth,
                                                uint64_t ordersSubmitted,
                                                uint64_t fillsReceived,
                                                uint64_t authFailures,
                                                uint64_t connectEvents,
                                                uint64_t disconnectEvents,
                                                uint64_t reconnectEvents)
{
    setGauge("aiio_network_handoff_p99_ms", networkHandoffP99Ms);
    setGauge("aiio_live_queue_depth", static_cast<double>(queueDepth));
    setGauge("aiio_orders_submitted_total", static_cast<double>(ordersSubmitted));
    setGauge("aiio_fills_received_total", static_cast<double>(fillsReceived));
    setGauge("aiio_auth_failures_total", static_cast<double>(authFailures));
    setGauge("aiio_connection_connect_total", static_cast<double>(connectEvents));
    setGauge("aiio_connection_disconnect_total", static_cast<double>(disconnectEvents));
    setGauge("aiio_connection_reconnect_total", static_cast<double>(reconnectEvents));
}

std::string LocalMetricsExporter::renderPrometheusText() const
{
    std::unordered_map<std::string, double> gauges;
    std::unordered_map<std::string, double> counters;
    {
        std::lock_guard<std::mutex> lock(m_metricsMutex);
        gauges = m_gauges;
        counters = m_counters;
    }

    std::ostringstream out;
    out.setf(std::ios::fixed);
    out.precision(6);

    for (const auto& kv : counters) {
        out << "# TYPE " << kv.first << " counter\n";
        out << kv.first << " " << kv.second << "\n";
    }
    for (const auto& kv : gauges) {
        out << "# TYPE " << kv.first << " gauge\n";
        out << kv.first << " " << kv.second << "\n";
    }
    return out.str();
}

uint64_t LocalMetricsExporter::totalScrapes() const noexcept
{
    return m_totalScrapes.load();
}

void LocalMetricsExporter::runServerLoop()
{
    while (m_running.load()) {
        sockaddr_in peer{};
        socklen_t peerLen = sizeof(peer);
        const int client = ::accept(m_listenFd, reinterpret_cast<sockaddr*>(&peer), &peerLen);
        if (client < 0) {
            if (!m_running.load()) { break; }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            continue;
        }

        std::array<char, 1024> reqBuf{};
        const ssize_t n = ::recv(client, reqBuf.data(), reqBuf.size(), 0);
        std::string req;
        if (n > 0) {
            req.assign(reqBuf.data(), static_cast<std::size_t>(n));
        }

        const bool metricsEndpoint = (req.find("GET /metrics") != std::string::npos)
            || (req.find("GET / ") != std::string::npos);

        std::string body;
        std::string status;
        if (metricsEndpoint) {
            body = renderPrometheusText();
            status = "HTTP/1.1 200 OK\r\n";
            m_totalScrapes.fetch_add(1);
        } else {
            body = "not found\n";
            status = "HTTP/1.1 404 Not Found\r\n";
        }

        std::ostringstream resp;
        resp << status
             << "Content-Type: text/plain; version=0.0.4\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << body;
        const std::string response = resp.str();
        (void)::send(client, response.data(), response.size(), 0);
        ::close(client);
    }
}

std::string LocalMetricsExporter::sanitizeMetricName(std::string name)
{
    if (name.empty()) { return {}; }
    for (char& ch : name) {
        const bool ok = std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' || ch == ':';
        if (!ok) {
            ch = '_';
        }
    }
    if (!(std::isalpha(static_cast<unsigned char>(name[0])) || name[0] == '_' || name[0] == ':')) {
        name.insert(name.begin(), '_');
    }
    return name;
}

void LocalMetricsExporter::setLastError(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_lastError = msg;
}

std::string LocalMetricsExporter::lastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}
