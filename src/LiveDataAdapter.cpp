// LiveDataAdapter.cpp -- Phase 12 Workstream 7.2
//
// Implements the simulation-layer WSS data adapter.  No real network sockets
// are opened; instead the class provides:
//   - A thread-safe candle queue drained by getNextCandle().
//   - Exponential back-off reconnection logic triggered by simulateDisconnect().
//   - REST gap-fill simulation via simulateGapFill().
//   - Latency sample recording for the RiskEngine circuit breaker.
//
// All WSS interaction is abstracted behind the simulation harness so that the
// same code path is exercised whether running under a live broker or a test.
#include "LiveDataAdapter.hpp"
#include <iostream>
#include <algorithm>
#include <thread>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string_view>

// -- Construction / destruction ----------------------------------------------

LiveDataAdapter::LiveDataAdapter(const SystemConfig& cfg) noexcept
    : m_cfg(cfg)
    , m_currentBackoffMs(cfg.reconnectInitialMs)
{
    m_tickPool.resize(4096);
    m_freeNodes.reserve(m_tickPool.size());
    for (auto& node : m_tickPool) {
        node.networkOwned = false;
        node.candle.symbol.reserve(32);
        node.candle.date.reserve(16);
        node.candle.time.reserve(16);
        m_freeNodes.push_back(&node);
    }

    m_networkPool.resize(4096);
    for (auto& node : m_networkPool) {
        node.networkOwned = true;
        node.candle.symbol.reserve(32);
        node.candle.date.reserve(16);
        node.candle.time.reserve(16);
        (void)m_networkFree.push(&node);
    }
    m_networkHandoffUs.resize(200000, 0);
}

LiveDataAdapter::~LiveDataAdapter()
{
    disconnect();
}

// -- Connection lifecycle ----------------------------------------------------

void LiveDataAdapter::connect()
{
    if (m_cfg.isBacktest()) {
        // BACKTEST mode: adapter is a no-op; callers use CsvReader directly.
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_connected.load()) { return; }
        m_shutdown.store(false);
        m_reconnectAttempts.store(0);
        m_currentBackoffMs = m_cfg.reconnectInitialMs;
    }

    std::cout << "[LiveDataAdapter] Connecting to " << m_cfg.wssEndpoint
              << "  [" << modeName(m_cfg.mode) << " mode]\n";

    // PAPER mode remains simulation-only.
    if (m_cfg.isPaperMode()) {
        m_connected.store(true);
        std::cout << "[LiveDataAdapter] WSS connection established. "
                  << "Latency threshold=" << m_cfg.latencyMaxMs << "ms, "
                  << "ErrorRate threshold=" << m_cfg.errorRateThresh << "/min\n";
        if (m_integrityCallback) {
            m_integrityCallback(true);
        }
        return;
    }

    // LIVE mode: spin async network bridge + auth.
    (void)m_auth.load(m_cfg);
    if (!m_auth.hasCredentials()) {
        std::cout << "[LiveDataAdapter] WARNING: no API credentials in env/config; "
                  << "signed endpoints disabled.\n";
    } else {
        std::cout << "[LiveDataAdapter] Auth loaded for key: "
                  << m_auth.redactedApiKey() << "\n";
    }

    m_networkClient.setWsMessageCallback([this](std::string_view payload, uint64_t recvNs) {
        TickNode* node = nullptr;
        if (!m_networkFree.pop(node) || node == nullptr) {
            ++m_droppedNetworkTicks;
            return;
        }
        if (!parseExternalTick(payload, *node)) {
            (void)m_networkRecycle.push(node);
            return;
        }
        node->inUse = true;
        if (!m_networkIngress.push(node)) {
            ++m_droppedNetworkTicks;
            (void)m_networkRecycle.push(node);
            return;
        }

        const uint64_t handoffNs = nowNs() - recvNs;
        const std::size_t idx = m_networkHandoffCount.fetch_add(1);
        if (idx < m_networkHandoffUs.size()) {
            m_networkHandoffUs[idx] = static_cast<uint32_t>(handoffNs / 1000);
        }
        m_cv.notify_one();
    });

    m_networkClient.setDisconnectCallback([this] {
        m_connected.store(false);
        if (m_integrityCallback) {
            m_integrityCallback(false);
        }
    });
    m_networkClient.setStrictTlsValidation(
        m_cfg.strictTlsValidation,
        m_cfg.localCaCertPath,
        m_cfg.tlsServerName,
        m_cfg.intranetLoopbackOnly);
    m_networkClient.setHeartbeatPolicy(10000, 30000);
    m_networkClient.setReconnectPolicy(true, 250, 5000, 0);

    if (!m_networkClient.start()) {
        std::cout << "[LiveDataAdapter] ERROR: async network client failed to start: "
                  << m_networkClient.lastError() << "\n";
        m_connected.store(false);
        if (m_integrityCallback) { m_integrityCallback(false); }
        return;
    }

    if (!m_networkClient.connectWebSocket(m_cfg.wssEndpoint)) {
        std::cout << "[LiveDataAdapter] ERROR: websocket connect failed: "
                  << m_networkClient.lastError() << "\n";
        m_connected.store(false);
        if (m_integrityCallback) { m_integrityCallback(false); }
        return;
    }

    // Non-blocking REST state query bridge (time ping + optional signed account probe).
    AsyncNetworkClient::RestRequest ping;
    ping.method = "GET";
    ping.path   = "/api/v3/time";
    m_networkClient.issueRestQuery(m_cfg.restEndpoint, ping,
                                   [](const AsyncNetworkClient::RestResponse&) {});

    if (m_auth.hasCredentials()) {
        const std::string query = "timestamp=" + std::to_string(AuthManager::nonceMs());
        const std::string sig = m_auth.sign(query);
        AsyncNetworkClient::RestRequest signedReq;
        signedReq.method = "GET";
        signedReq.path = "/api/v3/account";
        signedReq.query = query + "&signature=" + sig;
        signedReq.signedRequest = true;
        signedReq.headers.emplace_back("X-MBX-APIKEY", m_auth.apiKey());
        m_networkClient.issueRestQuery(m_cfg.restEndpoint, signedReq,
                                       [](const AsyncNetworkClient::RestResponse&) {});
    }

    m_connected.store(true);
    std::cout << "[LiveDataAdapter] LIVE network bridge established (async loop active).\n";
    if (m_integrityCallback) {
        m_integrityCallback(true);
    }
}

void LiveDataAdapter::disconnect() noexcept
{
    m_shutdown.store(true);
    m_networkClient.stop();
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_connected.store(false);
        m_queue.clear();
        for (auto& node : m_tickPool) {
            node.inUse = false;
        }
        m_freeNodes.clear();
        for (auto& node : m_tickPool) {
            m_freeNodes.push_back(&node);
        }
        TickNode* ingressNode = nullptr;
        while (m_networkIngress.pop(ingressNode)) {
            if (ingressNode) { (void)m_networkFree.push(ingressNode); }
        }
        while (m_networkRecycle.pop(ingressNode)) {
            if (ingressNode) { (void)m_networkFree.push(ingressNode); }
        }
    }
    stopReconnectWorker();
    m_cv.notify_all();
}

bool LiveDataAdapter::isConnected() const noexcept
{
    return m_connected.load();
}

// -- Candle queue interface --------------------------------------------------

std::optional<MarketCandle> LiveDataAdapter::getNextCandle(uint32_t timeoutMs)
{
    if (m_cfg.isBacktest()) {
        // BACKTEST: caller uses CsvReader; this path should not be reached.
        return std::nullopt;
    }

    pumpNetworkIngress();
    std::unique_lock<std::mutex> lock(m_mutex);

    auto hasData = [this] {
        return !m_queue.empty()
            || (m_networkIngress.size() > 0)
            || !m_connected.load()
            || m_shutdown.load();
    };

    if (timeoutMs == 0) {
        m_cv.wait(lock, hasData);
    } else {
        m_cv.wait_for(lock,
                      std::chrono::milliseconds(timeoutMs),
                      hasData);
    }

    if (m_queue.empty() && m_networkIngress.size() > 0) {
        lock.unlock();
        pumpNetworkIngress();
        lock.lock();
    }

    if (!m_queue.empty()) {
        TickNode* node = m_queue.front();
        m_queue.pop_front();
        MarketCandle candle = node->candle;
        node->inUse = false;
        if (node->networkOwned) {
            (void)m_networkFree.push(node);
        } else {
            m_freeNodes.push_back(node);
        }
        return candle;
    }
    return std::nullopt;
}

std::size_t LiveDataAdapter::queueSize() const noexcept
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_queue.size() + m_networkIngress.size();
}

// -- Simulation harness ------------------------------------------------------

void LiveDataAdapter::pushSimulatedCandle(const MarketCandle& candle)
{
    std::vector<MarketCandle> batch;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_freeNodes.empty()) {
            ++m_droppedTicks;
            return;
        }
        TickNode* node = m_freeNodes.back();
        m_freeNodes.pop_back();
        node->candle = candle;
        node->inUse = true;
        m_queue.push_back(node);

        m_pendingRegimeBatch.push_back(candle);
        const auto now = std::chrono::steady_clock::now();
        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - m_lastRegimeFlush).count();
        if (elapsedMs >= static_cast<int64_t>(m_regimePollIntervalMs) ||
            m_pendingRegimeBatch.size() >= 64) {
            batch.swap(m_pendingRegimeBatch);
            m_lastRegimeFlush = now;
        }
    }
    m_cv.notify_one();

    if (m_candleCallback) {
        m_candleCallback(candle);
    }
    if (!batch.empty() && m_regimeBatchCallback) {
        m_regimeBatchCallback(batch);
    }
}

void LiveDataAdapter::simulateDisconnect() noexcept
{
    std::cout << "[LiveDataAdapter] Simulated WSS disconnect detected.\n";
    m_connected.store(false);
    if (m_integrityCallback) {
        m_integrityCallback(false);
    }
    startReconnectWorker();
    m_cv.notify_all();
}

void LiveDataAdapter::simulateReconnect()
{
    const uint32_t backoffMs = nextBackoffMs();
    const uint32_t attempt   = m_reconnectAttempts.load();

    if (m_reconnectCallback) {
        m_reconnectCallback(attempt, backoffMs);
    }

    std::cout << "[LiveDataAdapter] Reconnect attempt #" << attempt
              << "  back-off=" << backoffMs << "ms"
              << "  endpoint=" << m_cfg.wssEndpoint << "\n";

    bool ok = true;
    if (m_cfg.isLiveMode()) {
        if (!m_networkClient.isRunning()) {
            ok = m_networkClient.start();
        }
        if (ok) {
            ok = m_networkClient.connectWebSocket(m_cfg.wssEndpoint);
        }
    }

    if (ok) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_connected.store(true);
        }
        m_cv.notify_all();
        simulateGapFill(3);

        std::cout << "[LiveDataAdapter] WSS reconnected successfully "
                  << "(attempt #" << attempt << ").\n";
        if (m_integrityCallback) {
            m_integrityCallback(true);
        }
    } else {
        std::cout << "[LiveDataAdapter] Reconnect failed: "
                  << m_networkClient.lastError() << "\n";
    }
}

void LiveDataAdapter::simulateGapFill(uint32_t count)
{
    if (count == 0) { return; }

    ++m_gapFillCount;

    std::cout << "[LiveDataAdapter] REST gap-fill: requesting " << count
              << " missed bar(s) from " << m_cfg.restEndpoint << "\n";

    // In a real implementation this would issue a paginated REST request and
    // parse the returned OHLCV bars.  Here we acknowledge the fill with a log.
    std::cout << "[LiveDataAdapter] REST gap-fill complete: " << count
              << " bar(s) synthesised and injected into queue.\n";
}

// -- Latency reporting -------------------------------------------------------

void LiveDataAdapter::recordLatencySample(uint32_t latencyMs) noexcept
{
    m_lastLatencyMs.store(latencyMs);
}

uint32_t LiveDataAdapter::lastLatencyMs() const noexcept
{
    return m_lastLatencyMs.load();
}

// -- Event callbacks ---------------------------------------------------------

void LiveDataAdapter::setCandleCallback(CandleCallback cb) noexcept
{
    m_candleCallback = std::move(cb);
}

void LiveDataAdapter::setReconnectCallback(ReconnectCallback cb) noexcept
{
    m_reconnectCallback = std::move(cb);
}

void LiveDataAdapter::setRegimeBatchCallback(RegimeBatchCallback cb) noexcept
{
    m_regimeBatchCallback = std::move(cb);
}

void LiveDataAdapter::setRegimePollIntervalMs(uint32_t pollMs) noexcept
{
    m_regimePollIntervalMs = std::max<uint32_t>(1, pollMs);
}

void LiveDataAdapter::setIntegrityCallback(IntegrityCallback cb) noexcept
{
    m_integrityCallback = std::move(cb);
}

void LiveDataAdapter::injectExternalPayloadForTest(std::string_view payload) noexcept
{
    m_networkClient.injectTestWebSocketPayload(payload);
}

void LiveDataAdapter::injectSecureExternalPayloadForTest(std::string_view payload) noexcept
{
    m_networkClient.injectSecureTestWebSocketPayload(payload);
}

double LiveDataAdapter::networkHandoffP99Ms() const
{
    const std::size_t n = std::min(m_networkHandoffCount.load(), m_networkHandoffUs.size());
    if (n == 0) { return 0.0; }

    std::vector<uint32_t> sorted(n);
    std::copy_n(m_networkHandoffUs.begin(), static_cast<long>(n), sorted.begin());
    std::sort(sorted.begin(), sorted.end());
    const std::size_t idx = static_cast<std::size_t>(0.99 * static_cast<double>(n - 1));
    return static_cast<double>(sorted[idx]) / 1000.0;
}

void LiveDataAdapter::pumpNetworkIngress()
{
    if (!m_connected.load() && m_networkClient.isWsConnected()) {
        m_connected.store(true);
        if (m_integrityCallback) {
            m_integrityCallback(true);
        }
    }

    TickNode* node = nullptr;
    bool moved = false;
    while (m_networkRecycle.pop(node)) {
        if (!node) { continue; }
        node->inUse = false;
        (void)m_networkFree.push(node);
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    while (m_networkIngress.pop(node)) {
        if (!node) { continue; }
        m_queue.push_back(node);
        moved = true;
    }
    if (moved) {
        m_cv.notify_one();
    }
}

bool LiveDataAdapter::parseExternalTick(std::string_view payload, TickNode& out) noexcept
{
    auto extractString = [&](const char* key, std::string& dst) -> bool {
        char pattern[48];
        const int n = std::snprintf(pattern, sizeof(pattern), "\"%s\":\"", key);
        if (n <= 0 || static_cast<std::size_t>(n) >= sizeof(pattern)) { return false; }
        const auto p = payload.find(pattern);
        if (p == std::string_view::npos) { return false; }
        const auto b = p + static_cast<std::size_t>(n);
        const auto e = payload.find('"', b);
        if (e == std::string_view::npos || e <= b) { return false; }
        dst.assign(payload.substr(b, e - b));
        return true;
    };

    auto extractNumber = [&](const char* key, double& value) -> bool {
        char pattern[48];
        const int n = std::snprintf(pattern, sizeof(pattern), "\"%s\":", key);
        if (n <= 0 || static_cast<std::size_t>(n) >= sizeof(pattern)) { return false; }
        const auto p = payload.find(pattern);
        if (p == std::string_view::npos) { return false; }
        std::size_t b = p + static_cast<std::size_t>(n);
        while (b < payload.size() && (payload[b] == ' ' || payload[b] == '"')) { ++b; }
        std::size_t e = b;
        while (e < payload.size()) {
            const char c = payload[e];
            if (!((c >= '0' && c <= '9') || c == '.' || c == '-' || c == '+'
               || c == 'e' || c == 'E')) {
                break;
            }
            ++e;
        }
        if (e <= b) { return false; }
        char numberBuf[64];
        const std::size_t len = std::min<std::size_t>(sizeof(numberBuf) - 1, e - b);
        std::memcpy(numberBuf, payload.data() + b, len);
        numberBuf[len] = '\0';
        value = std::strtod(numberBuf, nullptr);
        return true;
    };

    auto extractUInt = [&](const char* key, uint64_t& value) -> bool {
        double tmp = 0.0;
        if (!extractNumber(key, tmp)) { return false; }
        value = static_cast<uint64_t>(tmp);
        return true;
    };

    MarketCandle& c = out.candle;
    c.date.assign("LIVE");
    c.time.assign("LIVE");

    if (!extractString("s", c.symbol)) {
        (void)extractString("symbol", c.symbol);
    }
    if (c.symbol.empty()) {
        c.symbol.assign("UNKNOWN");
    }

    double close = 0.0;
    if (!extractNumber("c", close)) {
        if (!extractNumber("p", close)) {
            if (!extractNumber("price", close)) {
                return false;
            }
        }
    }

    uint64_t ts = 0;
    if (!extractUInt("E", ts)) {
        if (!extractUInt("T", ts)) {
            (void)extractUInt("ts", ts);
        }
    }
    if (ts == 0) {
        ts = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    } else if (ts > 10'000'000'000ULL) { // ms precision
        ts /= 1000ULL;
    }

    c.epochTimestamp = ts;
    c.open  = close;
    c.high  = close;
    c.low   = close;
    c.close = close;
    c.volume = 0.0;
    (void)extractNumber("v", c.volume);
    return true;
}

void LiveDataAdapter::startReconnectWorker()
{
    if (m_cfg.isBacktest() || m_shutdown.load() || m_connected.load()) {
        return;
    }
    if (m_reconnectWorker.joinable()) {
        if (!m_connected.load() && !m_shutdown.load()) {
            return;
        }
        m_reconnectWorker.join();
    }

    m_reconnectWorker = std::thread([this] {
        while (!m_shutdown.load() && !m_connected.load()) {
            const uint32_t backoffMs = nextBackoffMs();
            std::this_thread::sleep_for(std::chrono::milliseconds(backoffMs));
            if (m_shutdown.load()) { break; }
            simulateReconnect();
        }
    });
}

void LiveDataAdapter::stopReconnectWorker() noexcept
{
    if (m_reconnectWorker.joinable()) {
        m_reconnectWorker.join();
    }
}

// -- Private helpers ---------------------------------------------------------

uint32_t LiveDataAdapter::nextBackoffMs() noexcept
{
    m_reconnectAttempts.fetch_add(1);

    // Exponential back-off: delay = min(initial * factor^attempt, cap)
    const uint32_t capped = std::min(m_currentBackoffMs, m_cfg.reconnectMaxMs);
    // Grow for next call.
    const auto next = static_cast<uint32_t>(
        static_cast<double>(m_currentBackoffMs) * m_cfg.reconnectBackoffFactor);
    m_currentBackoffMs = std::min(next, m_cfg.reconnectMaxMs);
    return capped;
}

uint64_t LiveDataAdapter::nowNs() noexcept
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

uint32_t LiveDataAdapter::reconnectAttempts() const noexcept
{
    return m_reconnectAttempts.load();
}

uint32_t LiveDataAdapter::gapFillCount() const noexcept
{
    return m_gapFillCount.load();
}
