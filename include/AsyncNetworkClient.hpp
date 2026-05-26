#pragma once
#include "LockFreeRingBuffer.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

class AsyncNetworkClient {
public:
    enum class RestErrorClass : uint8_t {
        NONE = 0,
        TRANSPORT,
        AUTH,
        CLIENT,
        SERVER,
        RATE_LIMIT,
        PARSE
    };

    struct RestRequest {
        std::string method{"GET"};
        std::string path{"/"};
        std::string query;
        std::string body;
        std::vector<std::pair<std::string, std::string>> headers;
        bool        signedRequest{false};
        uint32_t    timeoutMs{5000};
    };

    struct RestResponse {
        int         statusCode{0};
        std::string body;
        uint64_t    recvTimestampNs{0};
        std::string error;
        RestErrorClass errorClass{RestErrorClass::NONE};
        bool ok() const noexcept { return error.empty() && statusCode >= 200 && statusCode < 300; }
    };

    using WsMessageCallback = std::function<void(std::string_view payload, uint64_t recvTimestampNs)>;
    using DisconnectCallback = std::function<void()>;
    using RestCallback = std::function<void(const RestResponse&)>;

    // Test hooks for deterministic REST hardening validation.
    static std::string buildRestHttpRequestForTest(const std::string& host, uint16_t port,
                                                   const RestRequest& req);
    static RestErrorClass classifyRestStatusForTest(int statusCode, bool signedRequest) noexcept;
    static bool validateCertificateChainForTest(const std::string& caCertPath,
                                                const std::string& certPath,
                                                std::string* errorOut = nullptr);

    AsyncNetworkClient() noexcept;
    ~AsyncNetworkClient();

    bool start();
    void stop() noexcept;

    bool connectWebSocket(const std::string& url);
    bool issueRestQuery(const std::string& baseUrl, const RestRequest& req, RestCallback cb);

    void setWsMessageCallback(WsMessageCallback cb) noexcept;
    void setDisconnectCallback(DisconnectCallback cb) noexcept;
    void setHeartbeatPolicy(uint32_t pingIntervalMs, uint32_t pongTimeoutMs) noexcept;
    void setReconnectPolicy(bool enabled, uint32_t initialBackoffMs, uint32_t maxBackoffMs,
                            uint32_t maxAttempts) noexcept;
    void setStrictTlsValidation(bool enabled, std::string caCertPath = {},
                                std::string expectedServerName = {},
                                bool intranetLoopbackOnly = true) noexcept;

    // Test harness path that exercises the same async event-loop callback flow.
    void injectTestWebSocketPayload(std::string_view payload) noexcept;
    void injectTestWebSocketFrame(std::string_view frameBytes) noexcept;
    void injectSecureTestWebSocketPayload(std::string_view payload) noexcept;

    bool isRunning() const noexcept;
    bool isWsConnected() const noexcept;
    uint64_t wsMessagesReceived() const noexcept;
    uint64_t wsBytesReceived() const noexcept;
    uint64_t wsPingsSent() const noexcept;
    uint64_t wsPongsReceived() const noexcept;
    uint64_t wsReconnectAttempts() const noexcept;
    uint64_t restRequestsIssued() const noexcept;
    std::string lastError() const;

private:
    enum class InjectedKind : uint8_t {
        PlainPayload = 0,
        RawFrame = 1,
        SecureFrame = 2
    };

    struct InjectedFrame {
        std::array<char, 1024> data{};
        uint16_t size{0};
        InjectedKind kind{InjectedKind::PlainPayload};
    };

    struct ParsedUrl {
        std::string scheme;
        std::string host;
        std::string path{"/"};
        uint16_t    port{0};
        bool        tls{false};
        bool        valid{false};
    };

    static ParsedUrl parseUrl(const std::string& url);
    void runLoop();
    void closeWsConnection() noexcept;
    void scheduleReconnect() noexcept;
    void notifyLoop() noexcept;
    void consumeInjectedFrame(const InjectedFrame& frame);
    void onSocketReadable();
    void processWsRxBuffer();
    void handleDisconnect();
    bool connectWebSocketInternal(const ParsedUrl& parsed);
    bool sendWsFrame(uint8_t opcode, std::string_view payload, bool maskClientFrame);
    void updateHeartbeat();
    void setLastError(const std::string& err);

    static std::string buildWsFrame(uint8_t opcode, std::string_view payload, bool maskClientFrame);
    static std::string xorSecureEnvelope(std::string_view data);
    static RestErrorClass classifyHttpStatus(int statusCode, bool signedRequest);
    static RestErrorClass classifyTransportError(const std::string& err) noexcept;
    static bool isLoopbackHost(std::string_view host) noexcept;
    static uint64_t nowNs() noexcept;

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopRequested{false};
    std::atomic<bool> m_wsConnected{false};

    std::thread m_thread;
    int m_epollFd{-1};
    int m_eventFd{-1};
    int m_wakeWriteFd{-1};
    int m_wsFd{-1};
    bool m_wsUsesTls{false};
    void* m_wsSslCtx{nullptr};
    void* m_wsSsl{nullptr};
    std::string m_wsUrl;
    ParsedUrl   m_wsParsedUrl;
    std::vector<char> m_wsRxBuffer;
    std::vector<char> m_wsFragmentBuffer;
    uint8_t m_wsFragmentOpcode{0};
    mutable std::mutex m_wsMutex;

    WsMessageCallback  m_wsCallback;
    DisconnectCallback m_disconnectCallback;
    mutable std::mutex m_callbackMutex;

    LockFreeRingBuffer<InjectedFrame, 2048> m_injectedFrames;

    std::atomic<uint64_t> m_wsMessagesReceived{0};
    std::atomic<uint64_t> m_wsBytesReceived{0};
    std::atomic<uint64_t> m_wsPingsSent{0};
    std::atomic<uint64_t> m_wsPongsReceived{0};
    std::atomic<uint64_t> m_wsReconnectAttempts{0};
    std::atomic<uint64_t> m_restRequestsIssued{0};
    std::atomic<uint64_t> m_lastWsRecvNs{0};
    std::atomic<uint64_t> m_lastWsPingNs{0};
    std::atomic<uint64_t> m_lastWsPongNs{0};

    std::atomic<bool>     m_reconnectEnabled{true};
    std::atomic<uint32_t> m_reconnectInitialBackoffMs{250};
    std::atomic<uint32_t> m_reconnectMaxBackoffMs{5000};
    std::atomic<uint32_t> m_reconnectMaxAttempts{8};
    std::atomic<uint32_t> m_reconnectCurrentBackoffMs{250};
    std::atomic<uint64_t> m_nextReconnectNs{0};
    std::atomic<uint32_t> m_heartbeatPingIntervalMs{15000};
    std::atomic<uint32_t> m_heartbeatPongTimeoutMs{45000};
    std::atomic<bool> m_tlsStrictValidation{false};
    std::atomic<bool> m_tlsIntranetLoopbackOnly{true};
    std::string m_tlsCaCertPath;
    std::string m_tlsExpectedServerName;
    mutable std::mutex m_tlsConfigMutex;

    mutable std::mutex m_errorMutex;
    std::string m_lastError;
};
