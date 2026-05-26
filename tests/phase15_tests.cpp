#include "AsyncNetworkClient.hpp"
#include "AuthManager.hpp"
#include "LiveDataAdapter.hpp"
#include "SystemConfig.hpp"

#include <atomic>
#include <chrono>
#include <cstdlib>
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

void test_hmac_sha256_known_vector()
{
    const std::string key = "key";
    const std::string msg = "The quick brown fox jumps over the lazy dog";
    const std::string sig = AuthManager::hmacSha256Hex(key, msg);
    require(sig == "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8",
            "HMAC-SHA256 signature mismatch for known vector");
}

void test_auth_load_from_env()
{
    ::setenv("AIIO_API_KEY", "PHASE15_TEST_KEY_1234", 1);
    ::setenv("AIIO_API_SECRET", "PHASE15_TEST_SECRET_5678", 1);

    SystemConfig cfg;
    cfg.apiKey.clear();
    cfg.apiSecret.clear();

    AuthManager auth;
    require(auth.load(cfg), "AuthManager failed to load env credentials");
    require(auth.hasCredentials(), "AuthManager missing credentials after env load");
    const std::string redacted = auth.redactedApiKey();
    require(redacted.find("****") != std::string::npos,
            "Redacted API key format invalid");
}

void test_async_network_client_injection_path()
{
    AsyncNetworkClient net;
    std::atomic<bool> seen{false};
    std::atomic<uint64_t> recvNs{0};
    std::string payloadSeen;

    net.setWsMessageCallback([&](std::string_view payload, uint64_t tsNs) {
        payloadSeen.assign(payload);
        recvNs.store(tsNs);
        seen.store(true);
    });

    std::cerr << "  [async] start\n";
    require(net.start(), "AsyncNetworkClient failed to start");
    std::cerr << "  [async] connect\n";
    require(net.connectWebSocket("mock://local-feed"), "AsyncNetworkClient mock ws connect failed");
    std::cerr << "  [async] inject\n";
    net.injectTestWebSocketPayload("{\"s\":\"BTCUSDT\",\"c\":\"42000.1\",\"E\":1710000000000}");

    const auto start = std::chrono::steady_clock::now();
    while (!seen.load()) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(1)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    std::cerr << "  [async] seen=" << seen.load() << "\n";
    require(seen.load(), "Async ws callback not observed");
    require(recvNs.load() > 0, "Async ws callback timestamp missing");
    require(payloadSeen.find("BTCUSDT") != std::string::npos,
            "Injected ws payload was not propagated");
    std::cerr << "  [async] stop\n";
    net.stop();
    std::cerr << "  [async] done\n";
}

void test_live_data_adapter_external_bridge()
{
    SystemConfig cfg;
    cfg.mode = SystemMode::LIVE;
    cfg.wssEndpoint = "mock://bench-feed";
    cfg.restEndpoint = "http://127.0.0.1:9";
    cfg.reconnectInitialMs = 10;
    cfg.reconnectMaxMs = 100;

    LiveDataAdapter adapter(cfg);
    adapter.connect();
    require(adapter.isConnected(), "LiveDataAdapter did not establish mock LIVE connection");

    constexpr int N = 200;
    for (int i = 0; i < N; ++i) {
        const uint64_t tsMs = 1710000000000ULL + static_cast<uint64_t>(i);
        const std::string payload = "{\"s\":\"BTCUSDT\",\"c\":\"42000.123\",\"v\":\"1.0\",\"E\":"
                                  + std::to_string(tsMs) + "}";
        adapter.injectExternalPayloadForTest(payload);
    }

    int got = 0;
    while (got < N) {
        const auto c = adapter.getNextCandle(50);
        if (!c.has_value()) { continue; }
        require(c->symbol == "BTCUSDT", "LiveDataAdapter parsed symbol mismatch");
        require(c->close > 0.0, "LiveDataAdapter parsed close price invalid");
        ++got;
    }

    const double p99Ms = adapter.networkHandoffP99Ms();
    require(p99Ms < 5.0, "Network->ring handoff p99 exceeded 5ms");
    adapter.disconnect();
}

} // namespace

int main()
{
    std::cerr << "[phase15_tests] test_hmac_sha256_known_vector...\n";
    test_hmac_sha256_known_vector();
    std::cerr << "[phase15_tests] test_auth_load_from_env...\n";
    test_auth_load_from_env();
    std::cerr << "[phase15_tests] test_async_network_client_injection_path...\n";
    test_async_network_client_injection_path();
    std::cerr << "[phase15_tests] test_live_data_adapter_external_bridge...\n";
    test_live_data_adapter_external_bridge();
    std::cout << "[phase15_tests] All tests passed.\n";
    return 0;
}
