#include "AsyncNetworkClient.hpp"
#include "LiveDataAdapter.hpp"
#include "SystemConfig.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

namespace {

void require(bool cond, const char* msg)
{
    if (!cond) {
        throw std::runtime_error(msg);
    }
}

std::string buildServerFrame(uint8_t opcode, std::string_view payload, bool fin = true)
{
    std::string out;
    out.reserve(payload.size() + 16);
    out.push_back(static_cast<char>((fin ? 0x80u : 0x00u) | (opcode & 0x0fu)));
    const std::size_t n = payload.size();
    if (n <= 125) {
        out.push_back(static_cast<char>(n));
    } else if (n <= 0xffff) {
        out.push_back(static_cast<char>(126));
        out.push_back(static_cast<char>((n >> 8) & 0xff));
        out.push_back(static_cast<char>(n & 0xff));
    } else {
        out.push_back(static_cast<char>(127));
        for (int i = 7; i >= 0; --i) {
            out.push_back(static_cast<char>((n >> (i * 8)) & 0xff));
        }
    }
    out.append(payload.data(), payload.size());
    return out;
}

void test_secure_payload_decryption_and_frame_parse()
{
    AsyncNetworkClient net;
    std::atomic<bool> seen{false};
    std::string payloadSeen;

    net.setWsMessageCallback([&](std::string_view payload, uint64_t) {
        payloadSeen.assign(payload);
        seen.store(true);
    });

    require(net.start(), "failed to start async network client");
    require(net.connectWebSocket("mock://phase16-secure-feed"), "mock websocket connect failed");
    net.injectSecureTestWebSocketPayload("{\"s\":\"BTCUSDT\",\"c\":\"62000.1\",\"E\":1710100000000}");

    const auto start = std::chrono::steady_clock::now();
    while (!seen.load()) {
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(1)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    require(seen.load(), "secure payload callback not observed");
    require(payloadSeen.find("BTCUSDT") != std::string::npos,
            "secure payload decryption/frame parse failed");
    require(net.wsMessagesReceived() >= 1, "ws message counter did not increment");
    net.stop();
}

void test_ws_ping_pong_fragment_close_semantics()
{
    AsyncNetworkClient net;
    std::atomic<bool> seenMessage{false};
    std::atomic<bool> seenDisconnect{false};
    std::string merged;

    net.setWsMessageCallback([&](std::string_view payload, uint64_t) {
        merged.assign(payload);
        seenMessage.store(true);
    });
    net.setDisconnectCallback([&] { seenDisconnect.store(true); });

    require(net.start(), "failed to start async network client");
    require(net.connectWebSocket("mock://phase16-ws-semantics"), "mock websocket connect failed");

    net.injectTestWebSocketFrame(buildServerFrame(0x1, "HELLO-", false));
    net.injectTestWebSocketFrame(buildServerFrame(0x0, "WORLD", true));

    const auto msgStart = std::chrono::steady_clock::now();
    while (!seenMessage.load()) {
        if (std::chrono::steady_clock::now() - msgStart > std::chrono::seconds(1)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    require(seenMessage.load(), "fragmented ws message was not reassembled");
    require(merged == "HELLO-WORLD", "fragmented ws payload mismatch");

    net.injectTestWebSocketFrame(buildServerFrame(0xA, "pong", true));
    const auto pongStart = std::chrono::steady_clock::now();
    while (net.wsPongsReceived() == 0) {
        if (std::chrono::steady_clock::now() - pongStart > std::chrono::seconds(1)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    require(net.wsPongsReceived() >= 1, "pong accounting not updated");

    net.injectTestWebSocketFrame(buildServerFrame(0x8, "", true));
    const auto discStart = std::chrono::steady_clock::now();
    while (!seenDisconnect.load()) {
        if (std::chrono::steady_clock::now() - discStart > std::chrono::seconds(1)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    require(seenDisconnect.load(), "disconnect callback not observed after close frame");
    require(!net.isWsConnected(), "client remained connected after close frame");
    require(net.wsReconnectAttempts() >= 1, "reconnect attempts counter did not increment");
    net.stop();
}

void test_rest_hardening_request_and_classification()
{
    AsyncNetworkClient::RestRequest req;
    req.method = "GET";
    req.path = "/api/v3/account";
    req.query = "timestamp=1&signature=abc";
    req.signedRequest = true;
    req.headers.emplace_back("X-MBX-APIKEY", "PHASE16_KEY");
    req.body = "";

    const std::string requestText = AsyncNetworkClient::buildRestHttpRequestForTest(
        "api.exchange.test", 443, req);
    require(requestText.find("X-MBX-APIKEY: PHASE16_KEY") != std::string::npos,
            "signed REST request builder missing API key header");
    require(requestText.find("GET /api/v3/account?timestamp=1&signature=abc HTTP/1.1")
            != std::string::npos,
            "REST request line format mismatch");

    const auto authClass = AsyncNetworkClient::classifyRestStatusForTest(401, true);
    require(authClass == AsyncNetworkClient::RestErrorClass::AUTH,
            "REST signed 401 classification mismatch");
    const auto rateClass = AsyncNetworkClient::classifyRestStatusForTest(429, true);
    require(rateClass == AsyncNetworkClient::RestErrorClass::RATE_LIMIT,
            "REST rate-limit classification mismatch");
}

void test_live_adapter_secure_bridge_p99()
{
    SystemConfig cfg;
    cfg.mode = SystemMode::LIVE;
    cfg.wssEndpoint = "mock://phase16-live-secure";
    cfg.restEndpoint = "http://127.0.0.1:9";
    cfg.reconnectInitialMs = 10;
    cfg.reconnectMaxMs = 100;

    LiveDataAdapter adapter(cfg);
    adapter.connect();
    require(adapter.isConnected(), "LiveDataAdapter mock LIVE connection failed");

    constexpr int N = 200;
    for (int i = 0; i < N; ++i) {
        const uint64_t tsMs = 1710100000000ULL + static_cast<uint64_t>(i);
        const std::string payload = "{\"s\":\"BTCUSDT\",\"c\":\"62000.321\",\"v\":\"2.0\",\"E\":"
                                  + std::to_string(tsMs) + "}";
        adapter.injectSecureExternalPayloadForTest(payload);
    }

    int got = 0;
    while (got < N) {
        const auto c = adapter.getNextCandle(50);
        if (!c.has_value()) { continue; }
        require(c->symbol == "BTCUSDT", "secure bridge parsed symbol mismatch");
        require(c->close > 0.0, "secure bridge parsed close invalid");
        ++got;
    }

    const double p99Ms = adapter.networkHandoffP99Ms();
    std::cerr << "[phase16_tests] networkHandoffP99Ms=" << p99Ms << "\n";
    require(p99Ms < 5.0, "network handoff p99 exceeded 5ms in secure bridge test");
    adapter.disconnect();
}

} // namespace

int main()
{
    std::cerr << "[phase16_tests] secure payload/frame parse...\n";
    test_secure_payload_decryption_and_frame_parse();
    std::cerr << "[phase16_tests] websocket semantics...\n";
    test_ws_ping_pong_fragment_close_semantics();
    std::cerr << "[phase16_tests] rest hardening...\n";
    test_rest_hardening_request_and_classification();
    std::cerr << "[phase16_tests] live adapter secure bridge + p99...\n";
    test_live_adapter_secure_bridge_p99();
    std::cout << "[phase16_tests] All tests passed.\n";
    return 0;
}
