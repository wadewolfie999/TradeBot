#include "AsyncNetworkClient.hpp"
#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <filesystem>
#include <fcntl.h>
#include <netdb.h>
#if defined(__linux__)
#include <sys/epoll.h>
#include <sys/eventfd.h>
#else
#include <poll.h>
#endif
#include <sstream>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

namespace {

constexpr uint8_t WS_OPCODE_CONTINUATION = 0x0;
constexpr uint8_t WS_OPCODE_TEXT         = 0x1;
constexpr uint8_t WS_OPCODE_BINARY       = 0x2;
constexpr uint8_t WS_OPCODE_CLOSE        = 0x8;
constexpr uint8_t WS_OPCODE_PING         = 0x9;
constexpr uint8_t WS_OPCODE_PONG         = 0xA;

constexpr int SSL_ERROR_NONE       = 0;
constexpr int SSL_ERROR_WANT_READ  = 2;
constexpr int SSL_ERROR_WANT_WRITE = 3;
constexpr int SSL_ERROR_ZERO_RETURN = 6;

using SSL_CTX_new_fn = void* (*)(const void*);
using SSL_CTX_free_fn = void (*)(void*);
using SSL_CTX_set_default_verify_paths_fn = int (*)(void*);
using SSL_CTX_load_verify_locations_fn = int (*)(void*, const char*, const char*);
using SSL_CTX_set_verify_fn = void (*)(void*, int, int (*)());
using SSL_new_fn = void* (*)(void*);
using SSL_free_fn = void (*)(void*);
using SSL_set_fd_fn = int (*)(void*, int);
using SSL_set_tlsext_host_name_fn = int (*)(void*, const char*);
using SSL_set1_host_fn = int (*)(void*, const char*);
using SSL_connect_fn = int (*)(void*);
using SSL_read_fn = int (*)(void*, void*, int);
using SSL_write_fn = int (*)(void*, const void*, int);
using SSL_shutdown_fn = int (*)(void*);
using SSL_get_error_fn = int (*)(const void*, int);
using SSL_get_verify_result_fn = long (*)(const void*);
using TLS_client_method_fn = const void* (*)();
using OPENSSL_init_ssl_fn = int (*)(uint64_t, const void*);
using ERR_get_error_fn = unsigned long (*)();
using ERR_error_string_n_fn = void (*)(unsigned long, char*, std::size_t);

constexpr int SSL_VERIFY_NONE = 0x00;
constexpr int SSL_VERIFY_PEER = 0x01;
constexpr long X509_V_OK = 0L;

class OpenSslShim {
public:
    static OpenSslShim& instance()
    {
        static OpenSslShim shim;
        return shim;
    }

    bool available() const noexcept
    {
        return m_loaded;
    }

    bool createClientContextAndSession(int fd, const std::string& host,
                                       bool strictValidation,
                                       const std::string& caCertPath,
                                       const std::string& expectedServerName,
                                       void*& outCtx, void*& outSsl,
                                       std::string& err)
    {
        outCtx = nullptr;
        outSsl = nullptr;
        if (!m_loaded) {
            err = "OpenSSL runtime unavailable";
            return false;
        }

        const void* method = tlsClientMethod();
        if (!method) {
            err = "TLS_client_method unavailable";
            return false;
        }

        void* ctx = sslCtxNew(method);
        if (!ctx) {
            err = "SSL_CTX_new failed: " + lastCryptoError();
            return false;
        }

        if (strictValidation) {
            if (caCertPath.empty()) {
                sslCtxFree(ctx);
                err = "Strict TLS validation enabled but CA path is empty";
                return false;
            }
            if (sslCtxLoadVerifyLocations(ctx, caCertPath.c_str(), nullptr) != 1) {
                sslCtxFree(ctx);
                err = "SSL_CTX_load_verify_locations failed: " + lastCryptoError();
                return false;
            }
            sslCtxSetVerify(ctx, SSL_VERIFY_PEER, nullptr);
        } else {
            (void)sslCtxSetDefaultVerifyPaths(ctx);
            sslCtxSetVerify(ctx, SSL_VERIFY_NONE, nullptr);
        }

        void* ssl = sslNew(ctx);
        if (!ssl) {
            sslCtxFree(ctx);
            err = "SSL_new failed: " + lastCryptoError();
            return false;
        }

        if (sslSetFd(ssl, fd) != 1) {
            sslFree(ssl);
            sslCtxFree(ctx);
            err = "SSL_set_fd failed: " + lastCryptoError();
            return false;
        }

        (void)sslSetSniHost(ssl, host.c_str());
        if (strictValidation && !expectedServerName.empty()) {
            if (!sslSet1Host) {
                sslFree(ssl);
                sslCtxFree(ctx);
                err = "SSL_set1_host unavailable for strict hostname validation";
                return false;
            }
            if (sslSet1Host(ssl, expectedServerName.c_str()) != 1) {
                sslFree(ssl);
                sslCtxFree(ctx);
                err = "SSL_set1_host failed: " + lastCryptoError();
                return false;
            }
        }
        if (sslConnect(ssl) != 1) {
            sslFree(ssl);
            sslCtxFree(ctx);
            err = "SSL_connect failed: " + lastCryptoError();
            return false;
        }
        if (strictValidation) {
            if (!sslGetVerifyResult) {
                sslFree(ssl);
                sslCtxFree(ctx);
                err = "SSL_get_verify_result unavailable";
                return false;
            }
            const long verifyResult = sslGetVerifyResult(ssl);
            if (verifyResult != X509_V_OK) {
                sslFree(ssl);
                sslCtxFree(ctx);
                err = "TLS peer certificate verification failed";
                return false;
            }
        }

        outCtx = ctx;
        outSsl = ssl;
        return true;
    }

    ssize_t read(void* ssl, void* dst, std::size_t n, bool nonBlocking, std::string& err)
    {
        if (!ssl || !m_loaded) {
            err = "SSL read without active TLS session";
            return -1;
        }
        const int rc = sslRead(ssl, dst, static_cast<int>(n));
        if (rc > 0) {
            return static_cast<ssize_t>(rc);
        }

        const int e = sslGetError(ssl, rc);
        if (e == SSL_ERROR_WANT_READ || e == SSL_ERROR_WANT_WRITE) {
            if (nonBlocking) {
                return 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return 0;
        }
        if (e == SSL_ERROR_ZERO_RETURN) {
            return -1;
        }

        err = "SSL_read failed: " + lastCryptoError();
        return -1;
    }

    ssize_t write(void* ssl, const void* src, std::size_t n, bool nonBlocking, std::string& err)
    {
        if (!ssl || !m_loaded) {
            err = "SSL write without active TLS session";
            return -1;
        }

        const int rc = sslWrite(ssl, src, static_cast<int>(n));
        if (rc > 0) {
            return static_cast<ssize_t>(rc);
        }

        const int e = sslGetError(ssl, rc);
        if (e == SSL_ERROR_WANT_READ || e == SSL_ERROR_WANT_WRITE) {
            if (nonBlocking) {
                return 0;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return 0;
        }

        err = "SSL_write failed: " + lastCryptoError();
        return -1;
    }

    void shutdownAndFree(void*& ssl, void*& sslCtx) noexcept
    {
        if (!m_loaded) {
            ssl = nullptr;
            sslCtx = nullptr;
            return;
        }
        if (ssl) {
            (void)sslShutdown(ssl);
            sslFree(ssl);
            ssl = nullptr;
        }
        if (sslCtx) {
            sslCtxFree(sslCtx);
            sslCtx = nullptr;
        }
    }

private:
    OpenSslShim()
    {
        m_sslHandle = ::dlopen("libssl.so.3", RTLD_NOW | RTLD_LOCAL);
        m_cryptoHandle = ::dlopen("libcrypto.so.3", RTLD_NOW | RTLD_LOCAL);
        if (!m_sslHandle || !m_cryptoHandle) {
            m_loaded = false;
            return;
        }

        openSslInit = resolve<OPENSSL_init_ssl_fn>(m_sslHandle, "OPENSSL_init_ssl");
        tlsClientMethod = resolve<TLS_client_method_fn>(m_sslHandle, "TLS_client_method");
        sslCtxNew = resolve<SSL_CTX_new_fn>(m_sslHandle, "SSL_CTX_new");
        sslCtxFree = resolve<SSL_CTX_free_fn>(m_sslHandle, "SSL_CTX_free");
        sslCtxSetDefaultVerifyPaths =
            resolve<SSL_CTX_set_default_verify_paths_fn>(m_sslHandle, "SSL_CTX_set_default_verify_paths");
        sslCtxLoadVerifyLocations =
            resolve<SSL_CTX_load_verify_locations_fn>(m_sslHandle, "SSL_CTX_load_verify_locations");
        sslCtxSetVerify = resolve<SSL_CTX_set_verify_fn>(m_sslHandle, "SSL_CTX_set_verify");
        sslNew = resolve<SSL_new_fn>(m_sslHandle, "SSL_new");
        sslFree = resolve<SSL_free_fn>(m_sslHandle, "SSL_free");
        sslSetFd = resolve<SSL_set_fd_fn>(m_sslHandle, "SSL_set_fd");
        sslSetSniHost = resolve<SSL_set_tlsext_host_name_fn>(m_sslHandle, "SSL_set_tlsext_host_name");
        sslSet1Host = resolve<SSL_set1_host_fn>(m_sslHandle, "SSL_set1_host");
        sslConnect = resolve<SSL_connect_fn>(m_sslHandle, "SSL_connect");
        sslRead = resolve<SSL_read_fn>(m_sslHandle, "SSL_read");
        sslWrite = resolve<SSL_write_fn>(m_sslHandle, "SSL_write");
        sslShutdown = resolve<SSL_shutdown_fn>(m_sslHandle, "SSL_shutdown");
        sslGetError = resolve<SSL_get_error_fn>(m_sslHandle, "SSL_get_error");
        sslGetVerifyResult = resolve<SSL_get_verify_result_fn>(m_sslHandle, "SSL_get_verify_result");
        errGetError = resolve<ERR_get_error_fn>(m_cryptoHandle, "ERR_get_error");
        errErrorStringN = resolve<ERR_error_string_n_fn>(m_cryptoHandle, "ERR_error_string_n");

        m_loaded = openSslInit && tlsClientMethod && sslCtxNew && sslCtxFree
            && sslCtxSetDefaultVerifyPaths && sslCtxLoadVerifyLocations && sslCtxSetVerify
            && sslNew && sslFree && sslSetFd && sslSetSniHost && sslConnect && sslRead
            && sslWrite && sslShutdown && sslGetError && sslGetVerifyResult
            && errGetError && errErrorStringN;
        if (m_loaded) {
            (void)openSslInit(0, nullptr);
        }
    }

    ~OpenSslShim()
    {
        if (m_sslHandle) {
            ::dlclose(m_sslHandle);
            m_sslHandle = nullptr;
        }
        if (m_cryptoHandle) {
            ::dlclose(m_cryptoHandle);
            m_cryptoHandle = nullptr;
        }
    }

    template <typename T>
    static T resolve(void* handle, const char* symbolName)
    {
        if (!handle) { return nullptr; }
        return reinterpret_cast<T>(::dlsym(handle, symbolName));
    }

    std::string lastCryptoError() const
    {
        if (!errGetError || !errErrorStringN) {
            return "unknown";
        }
        const unsigned long code = errGetError();
        if (code == 0UL) {
            return "unknown";
        }
        std::array<char, 256> buf{};
        errErrorStringN(code, buf.data(), buf.size());
        return std::string(buf.data());
    }

    void* m_sslHandle{nullptr};
    void* m_cryptoHandle{nullptr};
    bool m_loaded{false};

    OPENSSL_init_ssl_fn openSslInit{nullptr};
    TLS_client_method_fn tlsClientMethod{nullptr};
    SSL_CTX_new_fn sslCtxNew{nullptr};
    SSL_CTX_free_fn sslCtxFree{nullptr};
    SSL_CTX_set_default_verify_paths_fn sslCtxSetDefaultVerifyPaths{nullptr};
    SSL_CTX_load_verify_locations_fn sslCtxLoadVerifyLocations{nullptr};
    SSL_CTX_set_verify_fn sslCtxSetVerify{nullptr};
    SSL_new_fn sslNew{nullptr};
    SSL_free_fn sslFree{nullptr};
    SSL_set_fd_fn sslSetFd{nullptr};
    SSL_set_tlsext_host_name_fn sslSetSniHost{nullptr};
    SSL_set1_host_fn sslSet1Host{nullptr};
    SSL_connect_fn sslConnect{nullptr};
    SSL_read_fn sslRead{nullptr};
    SSL_write_fn sslWrite{nullptr};
    SSL_shutdown_fn sslShutdown{nullptr};
    SSL_get_error_fn sslGetError{nullptr};
    SSL_get_verify_result_fn sslGetVerifyResult{nullptr};
    ERR_get_error_fn errGetError{nullptr};
    ERR_error_string_n_fn errErrorStringN{nullptr};
};

bool setNonBlocking(int fd, bool enabled)
{
    const int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0) { return false; }
    const int nextFlags = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return ::fcntl(fd, F_SETFL, nextFlags) == 0;
}

void setSocketTimeouts(int fd, uint32_t timeoutMs)
{
    const timeval tv{
        static_cast<time_t>(timeoutMs / 1000),
        static_cast<suseconds_t>((timeoutMs % 1000) * 1000)
    };
    (void)::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    (void)::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

int connectTcpBlocking(const std::string& host, uint16_t port, uint32_t timeoutMs, std::string& errorOut)
{
    struct addrinfo hints {};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = nullptr;
    const std::string portStr = std::to_string(port);
    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0) {
        errorOut = "DNS resolution failed for host: " + host;
        return -1;
    }

    int fd = -1;
    for (auto* rp = result; rp != nullptr; rp = rp->ai_next) {
        fd = ::socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) { continue; }
        setSocketTimeouts(fd, timeoutMs);
        if (::connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        ::close(fd);
        fd = -1;
    }
    freeaddrinfo(result);

    if (fd < 0) {
        errorOut = "TCP connect failed: " + std::string(std::strerror(errno));
    }
    return fd;
}

bool plainSendAll(int fd, std::string_view data, std::string& err)
{
    std::size_t sent = 0;
    while (sent < data.size()) {
        const ssize_t n = ::send(fd, data.data() + sent, data.size() - sent, 0);
        if (n > 0) {
            sent += static_cast<std::size_t>(n);
            continue;
        }
        if (errno == EINTR) { continue; }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        err = "send failed: " + std::string(std::strerror(errno));
        return false;
    }
    return true;
}

bool tlsSendAll(void* ssl, std::string_view data, std::string& err)
{
    OpenSslShim& tls = OpenSslShim::instance();
    std::size_t sent = 0;
    while (sent < data.size()) {
        const ssize_t n = tls.write(ssl, data.data() + sent, data.size() - sent, false, err);
        if (n > 0) {
            sent += static_cast<std::size_t>(n);
            continue;
        }
        if (n == 0) {
            continue;
        }
        return false;
    }
    return true;
}

bool readHttpHeader(int fd, bool tlsEnabled, void* ssl, std::string& headerOut, std::string& err)
{
    constexpr std::size_t kMaxHeader = 64 * 1024;
    headerOut.clear();
    std::array<char, 4096> buf{};
    OpenSslShim& tls = OpenSslShim::instance();

    while (headerOut.find("\r\n\r\n") == std::string::npos) {
        ssize_t n = 0;
        if (tlsEnabled) {
            n = tls.read(ssl, buf.data(), buf.size(), false, err);
        } else {
            n = ::recv(fd, buf.data(), buf.size(), 0);
            if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            if (n < 0) {
                err = "recv failed: " + std::string(std::strerror(errno));
            }
        }

        if (n <= 0) {
            if (err.empty()) {
                err = "peer closed during handshake";
            }
            return false;
        }
        headerOut.append(buf.data(), static_cast<std::size_t>(n));
        if (headerOut.size() > kMaxHeader) {
            err = "HTTP header exceeded limit";
            return false;
        }
    }
    return true;
}

std::string recvAllHttpBodyPlain(int fd)
{
    std::string response;
    std::array<char, 4096> buffer{};
    while (true) {
        const ssize_t n = ::recv(fd, buffer.data(), buffer.size(), 0);
        if (n > 0) {
            response.append(buffer.data(), static_cast<std::size_t>(n));
            continue;
        }
        if (n < 0 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        break;
    }
    return response;
}

std::string recvAllHttpBodyTls(void* ssl)
{
    std::string response;
    std::array<char, 4096> buffer{};
    OpenSslShim& tls = OpenSslShim::instance();
    std::string err;
    while (true) {
        const ssize_t n = tls.read(ssl, buffer.data(), buffer.size(), false, err);
        if (n > 0) {
            response.append(buffer.data(), static_cast<std::size_t>(n));
            continue;
        }
        if (n == 0) {
            continue;
        }
        break;
    }
    return response;
}

std::string shellEscapeSingleQuoted(std::string_view text)
{
    std::string out;
    out.reserve(text.size() + 8);
    for (char c : text) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out.push_back(c);
        }
    }
    return out;
}

} // namespace

AsyncNetworkClient::AsyncNetworkClient() noexcept = default;

AsyncNetworkClient::~AsyncNetworkClient()
{
    stop();
}

bool AsyncNetworkClient::start()
{
    if (m_running.load()) { return true; }

#if defined(__linux__)
    m_epollFd = ::epoll_create1(EPOLL_CLOEXEC);
    if (m_epollFd < 0) {
        setLastError("epoll_create1 failed");
        return false;
    }

    m_eventFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_eventFd < 0) {
        setLastError("eventfd create failed");
        ::close(m_epollFd);
        m_epollFd = -1;
        return false;
    }

    epoll_event ev {};
    ev.events = EPOLLIN;
    ev.data.fd = m_eventFd;
    if (::epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_eventFd, &ev) != 0) {
        setLastError("epoll_ctl(ADD eventfd) failed");
        ::close(m_eventFd);
        ::close(m_epollFd);
        m_eventFd = -1;
        m_epollFd = -1;
        return false;
    }
#else
    int wakePipe[2] = {-1, -1};
    if (::pipe(wakePipe) != 0) {
        setLastError("wake pipe create failed");
        return false;
    }
    if (!setNonBlocking(wakePipe[0], true) || !setNonBlocking(wakePipe[1], true)) {
        setLastError("wake pipe non-blocking setup failed");
        ::close(wakePipe[0]);
        ::close(wakePipe[1]);
        return false;
    }
    (void)::fcntl(wakePipe[0], F_SETFD, FD_CLOEXEC);
    (void)::fcntl(wakePipe[1], F_SETFD, FD_CLOEXEC);
    m_eventFd = wakePipe[0];
    m_wakeWriteFd = wakePipe[1];
#endif

    m_stopRequested.store(false);
    m_running.store(true);
    m_thread = std::thread([this] { runLoop(); });
    return true;
}

void AsyncNetworkClient::stop() noexcept
{
    if (!m_running.load()) { return; }
    m_stopRequested.store(true);
    notifyLoop();

    if (m_thread.joinable()) {
        m_thread.join();
    }

    closeWsConnection();

    if (m_eventFd >= 0) {
        ::close(m_eventFd);
        m_eventFd = -1;
    }
    if (m_wakeWriteFd >= 0) {
        ::close(m_wakeWriteFd);
        m_wakeWriteFd = -1;
    }
    if (m_epollFd >= 0) {
        ::close(m_epollFd);
        m_epollFd = -1;
    }

    m_wsConnected.store(false);
    m_running.store(false);
}

AsyncNetworkClient::ParsedUrl AsyncNetworkClient::parseUrl(const std::string& url)
{
    ParsedUrl out;
    const auto schemePos = url.find("://");
    if (schemePos == std::string::npos) { return out; }

    out.scheme = url.substr(0, schemePos);
    std::string rem = url.substr(schemePos + 3);

    const auto pathPos = rem.find('/');
    if (pathPos != std::string::npos) {
        out.path = rem.substr(pathPos);
        rem      = rem.substr(0, pathPos);
    } else {
        out.path = "/";
    }

    const auto colonPos = rem.find(':');
    if (colonPos != std::string::npos) {
        out.host = rem.substr(0, colonPos);
        out.port = static_cast<uint16_t>(std::stoi(rem.substr(colonPos + 1)));
    } else {
        out.host = rem;
        if (out.scheme == "ws")      { out.port = 80; }
        if (out.scheme == "wss")     { out.port = 443; out.tls = true; }
        if (out.scheme == "http")    { out.port = 80; }
        if (out.scheme == "https")   { out.port = 443; out.tls = true; }
        if (out.scheme == "mock")    { out.port = 0; }
    }

    if (out.scheme == "wss" || out.scheme == "https") {
        out.tls = true;
    }
    out.valid = !out.host.empty() || out.scheme == "mock";
    return out;
}

bool AsyncNetworkClient::connectWebSocket(const std::string& url)
{
    if (!m_running.load() && !start()) {
        return false;
    }

    const ParsedUrl parsed = parseUrl(url);
    if (!parsed.valid) {
        setLastError("Invalid websocket URL: " + url);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_wsMutex);
        m_wsUrl = url;
        m_wsParsedUrl = parsed;
    }

    return connectWebSocketInternal(parsed);
}

bool AsyncNetworkClient::connectWebSocketInternal(const ParsedUrl& parsed)
{
    closeWsConnection();

    if (parsed.scheme == "mock") {
        m_wsConnected.store(true);
        const uint64_t now = nowNs();
        m_lastWsRecvNs.store(now);
        m_lastWsPongNs.store(now);
        m_lastWsPingNs.store(0);
        m_reconnectCurrentBackoffMs.store(m_reconnectInitialBackoffMs.load());
        m_nextReconnectNs.store(0);
        return true;
    }

    if (parsed.tls && m_tlsStrictValidation.load()
        && m_tlsIntranetLoopbackOnly.load()
        && !isLoopbackHost(parsed.host)) {
        setLastError("Strict INTRANET TLS mode rejects non-loopback host: " + parsed.host);
        return false;
    }

    std::string connectErr;
    const int fd = connectTcpBlocking(parsed.host, parsed.port, 5000, connectErr);
    if (fd < 0) {
        setLastError(connectErr);
        return false;
    }

    void* sslCtx = nullptr;
    void* ssl = nullptr;
    if (parsed.tls) {
        bool strictTls = m_tlsStrictValidation.load();
        std::string caPath;
        std::string expectedName;
        {
            std::lock_guard<std::mutex> lock(m_tlsConfigMutex);
            caPath = m_tlsCaCertPath;
            expectedName = m_tlsExpectedServerName;
        }
        if (strictTls && expectedName.empty()) {
            expectedName = parsed.host;
        }
        OpenSslShim& tls = OpenSslShim::instance();
        if (!tls.available()) {
            ::close(fd);
            setLastError("TLS unavailable: OpenSSL runtime symbols not present");
            return false;
        }
        if (!tls.createClientContextAndSession(fd, parsed.host, strictTls,
                                               caPath, expectedName, sslCtx, ssl, connectErr)) {
            ::close(fd);
            setLastError(connectErr);
            return false;
        }
    }

    std::ostringstream hs;
    hs << "GET " << parsed.path << " HTTP/1.1\r\n"
       << "Host: " << parsed.host << ":" << parsed.port << "\r\n"
       << "Upgrade: websocket\r\n"
       << "Connection: Upgrade\r\n"
       << "Sec-WebSocket-Version: 13\r\n"
       << "Sec-WebSocket-Key: SGVsbG9Xb3JsZEJlbmNoa2V5IQ==\r\n\r\n";
    const std::string hsText = hs.str();

    bool sentOk = false;
    if (parsed.tls) {
        sentOk = tlsSendAll(ssl, hsText, connectErr);
    } else {
        sentOk = plainSendAll(fd, hsText, connectErr);
    }
    if (!sentOk) {
        OpenSslShim::instance().shutdownAndFree(ssl, sslCtx);
        ::close(fd);
        setLastError(connectErr);
        return false;
    }

    std::string handshakeResp;
    if (!readHttpHeader(fd, parsed.tls, ssl, handshakeResp, connectErr)) {
        OpenSslShim::instance().shutdownAndFree(ssl, sslCtx);
        ::close(fd);
        setLastError("WebSocket handshake failed: " + connectErr);
        return false;
    }

    if (handshakeResp.find(" 101 ") == std::string::npos) {
        OpenSslShim::instance().shutdownAndFree(ssl, sslCtx);
        ::close(fd);
        setLastError("WebSocket upgrade rejected by remote endpoint");
        return false;
    }

    if (!setNonBlocking(fd, true)) {
        OpenSslShim::instance().shutdownAndFree(ssl, sslCtx);
        ::close(fd);
        setLastError("Failed to set non-blocking websocket socket");
        return false;
    }

#if defined(__linux__)
    epoll_event ev {};
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
    ev.data.fd = fd;
    if (::epoll_ctl(m_epollFd, EPOLL_CTL_ADD, fd, &ev) != 0) {
        OpenSslShim::instance().shutdownAndFree(ssl, sslCtx);
        ::close(fd);
        setLastError("epoll_ctl(ADD ws fd) failed");
        return false;
    }
#endif

    {
        std::lock_guard<std::mutex> lock(m_wsMutex);
        m_wsFd = fd;
        m_wsUsesTls = parsed.tls;
        m_wsSslCtx = sslCtx;
        m_wsSsl = ssl;
        m_wsRxBuffer.clear();
        m_wsFragmentBuffer.clear();
        m_wsFragmentOpcode = 0;
    }

    m_wsConnected.store(true);
    const uint64_t now = nowNs();
    m_lastWsRecvNs.store(now);
    m_lastWsPongNs.store(now);
    m_lastWsPingNs.store(0);
    m_reconnectCurrentBackoffMs.store(m_reconnectInitialBackoffMs.load());
    m_nextReconnectNs.store(0);
    return true;
}

void AsyncNetworkClient::closeWsConnection() noexcept
{
    std::lock_guard<std::mutex> lock(m_wsMutex);
#if defined(__linux__)
    if (m_epollFd >= 0 && m_wsFd >= 0) {
        (void)::epoll_ctl(m_epollFd, EPOLL_CTL_DEL, m_wsFd, nullptr);
    }
#endif
    if (m_wsFd >= 0) {
        ::close(m_wsFd);
        m_wsFd = -1;
    }
    OpenSslShim::instance().shutdownAndFree(m_wsSsl, m_wsSslCtx);
    m_wsUsesTls = false;
    m_wsRxBuffer.clear();
    m_wsFragmentBuffer.clear();
    m_wsFragmentOpcode = 0;
    m_wsConnected.store(false);
}

void AsyncNetworkClient::scheduleReconnect() noexcept
{
    if (!m_reconnectEnabled.load() || m_stopRequested.load()) {
        return;
    }
    if (m_wsUrl.empty() || m_wsParsedUrl.scheme == "mock") {
        return;
    }

    const uint32_t maxAttempts = m_reconnectMaxAttempts.load();
    const uint64_t attempts = m_wsReconnectAttempts.load();
    if (maxAttempts > 0 && attempts >= maxAttempts) {
        return;
    }

    const uint32_t backoffMs = std::min(m_reconnectCurrentBackoffMs.load(),
                                        m_reconnectMaxBackoffMs.load());
    m_nextReconnectNs.store(nowNs() + static_cast<uint64_t>(backoffMs) * 1'000'000ULL);
    m_reconnectCurrentBackoffMs.store(
        std::min(backoffMs * 2, m_reconnectMaxBackoffMs.load()));
}

AsyncNetworkClient::RestErrorClass AsyncNetworkClient::classifyHttpStatus(
    int statusCode, bool signedRequest)
{
    if (statusCode >= 200 && statusCode < 300) {
        return RestErrorClass::NONE;
    }
    if (statusCode == 401 || statusCode == 403) {
        return signedRequest ? RestErrorClass::AUTH : RestErrorClass::CLIENT;
    }
    if (statusCode == 429) {
        return RestErrorClass::RATE_LIMIT;
    }
    if (statusCode >= 400 && statusCode < 500) {
        return RestErrorClass::CLIENT;
    }
    if (statusCode >= 500) {
        return RestErrorClass::SERVER;
    }
    return RestErrorClass::PARSE;
}

std::string AsyncNetworkClient::buildRestHttpRequestForTest(const std::string& host, uint16_t port,
                                                            const RestRequest& req)
{
    std::ostringstream request;
    request << req.method << " " << req.path;
    if (!req.query.empty()) { request << "?" << req.query; }
    request << " HTTP/1.1\r\n"
            << "Host: " << host << ":" << port << "\r\n"
            << "Connection: close\r\n"
            << "User-Agent: AIIO-AsyncNetworkClient/16.0\r\n";

    bool hasApiKeyHeader = false;
    for (const auto& kv : req.headers) {
        if (kv.first == "X-MBX-APIKEY") {
            hasApiKeyHeader = true;
        }
        request << kv.first << ": " << kv.second << "\r\n";
    }
    if (req.signedRequest && !hasApiKeyHeader) {
        request << "X-MBX-APIKEY: \r\n";
    }
    if (!req.body.empty()) {
        request << "Content-Length: " << req.body.size() << "\r\n";
    }
    request << "\r\n";
    if (!req.body.empty()) {
        request << req.body;
    }
    return request.str();
}

AsyncNetworkClient::RestErrorClass AsyncNetworkClient::classifyRestStatusForTest(
    int statusCode, bool signedRequest) noexcept
{
    return classifyHttpStatus(statusCode, signedRequest);
}

bool AsyncNetworkClient::validateCertificateChainForTest(const std::string& caCertPath,
                                                         const std::string& certPath,
                                                         std::string* errorOut)
{
    if (!std::filesystem::exists(caCertPath)) {
        if (errorOut) { *errorOut = "CA certificate file not found"; }
        return false;
    }
    if (!std::filesystem::exists(certPath)) {
        if (errorOut) { *errorOut = "Target certificate file not found"; }
        return false;
    }

    const std::string cmd = "openssl verify -CAfile '"
        + shellEscapeSingleQuoted(caCertPath) + "' '"
        + shellEscapeSingleQuoted(certPath) + "' > /dev/null 2>&1";
    const int rc = std::system(cmd.c_str());
    if (rc != 0) {
        if (errorOut) { *errorOut = "openssl verify rejected certificate"; }
        return false;
    }
    if (errorOut) { errorOut->clear(); }
    return true;
}

AsyncNetworkClient::RestErrorClass AsyncNetworkClient::classifyTransportError(
    const std::string& err) noexcept
{
    if (err.find("auth") != std::string::npos ||
        err.find("signature") != std::string::npos) {
        return RestErrorClass::AUTH;
    }
    if (err.find("timeout") != std::string::npos ||
        err.find("connect") != std::string::npos ||
        err.find("TLS") != std::string::npos ||
        err.find("recv") != std::string::npos ||
        err.find("send") != std::string::npos) {
        return RestErrorClass::TRANSPORT;
    }
    return RestErrorClass::PARSE;
}

bool AsyncNetworkClient::issueRestQuery(const std::string& baseUrl,
                                        const RestRequest& req,
                                        RestCallback cb)
{
    m_restRequestsIssued.fetch_add(1);
    const ParsedUrl parsed = parseUrl(baseUrl);
    if (!parsed.valid) {
        if (cb) {
            cb(RestResponse{
                0, "", nowNs(), "Invalid REST base URL", RestErrorClass::PARSE
            });
        }
        return false;
    }

    const bool strictTls = m_tlsStrictValidation.load();
    const bool loopbackOnly = m_tlsIntranetLoopbackOnly.load();
    std::string caPath;
    std::string expectedName;
    {
        std::lock_guard<std::mutex> lock(m_tlsConfigMutex);
        caPath = m_tlsCaCertPath;
        expectedName = m_tlsExpectedServerName;
    }
    if (strictTls && expectedName.empty()) {
        expectedName = parsed.host;
    }
    if (parsed.tls && strictTls && loopbackOnly && !isLoopbackHost(parsed.host)) {
        if (cb) {
            cb(RestResponse{
                0, "", nowNs(),
                "Strict INTRANET TLS mode rejects non-loopback host",
                RestErrorClass::AUTH
            });
        }
        return false;
    }

    std::thread([parsed, req, cb = std::move(cb),
                 strictTls,
                 caPath = std::move(caPath),
                 expectedName = std::move(expectedName)]() {
        std::string err;
        const int fd = connectTcpBlocking(parsed.host, parsed.port,
                                          std::max<uint32_t>(50U, req.timeoutMs), err);
        if (fd < 0) {
            if (cb) {
                cb(RestResponse{
                    0, "", AsyncNetworkClient::nowNs(), err,
                    AsyncNetworkClient::classifyTransportError(err)
                });
            }
            return;
        }

        void* sslCtx = nullptr;
        void* ssl = nullptr;
        if (parsed.tls) {
            OpenSslShim& tls = OpenSslShim::instance();
            if (!tls.available()) {
                ::close(fd);
                if (cb) {
                    cb(RestResponse{
                        0, "", AsyncNetworkClient::nowNs(),
                        "TLS unavailable: OpenSSL runtime symbols not present",
                        AsyncNetworkClient::RestErrorClass::TRANSPORT
                    });
                }
                return;
            }
            if (!tls.createClientContextAndSession(fd, parsed.host, strictTls,
                                                   caPath, expectedName, sslCtx, ssl, err)) {
                ::close(fd);
                if (cb) {
                    cb(RestResponse{
                        0, "", AsyncNetworkClient::nowNs(), err,
                        AsyncNetworkClient::classifyTransportError(err)
                    });
                }
                return;
            }
        }

        const std::string reqText = AsyncNetworkClient::buildRestHttpRequestForTest(
            parsed.host, parsed.port, req);
        bool sendOk = false;
        if (parsed.tls) {
            sendOk = tlsSendAll(ssl, reqText, err);
        } else {
            sendOk = plainSendAll(fd, reqText, err);
        }
        if (!sendOk) {
            OpenSslShim::instance().shutdownAndFree(ssl, sslCtx);
            ::close(fd);
            if (cb) {
                cb(RestResponse{
                    0, "", AsyncNetworkClient::nowNs(), err,
                    AsyncNetworkClient::classifyTransportError(err)
                });
            }
            return;
        }

        std::string respText = parsed.tls
            ? recvAllHttpBodyTls(ssl)
            : recvAllHttpBodyPlain(fd);
        OpenSslShim::instance().shutdownAndFree(ssl, sslCtx);
        ::close(fd);

        int statusCode = 0;
        const auto firstSp = respText.find(' ');
        if (firstSp != std::string::npos && firstSp + 4 <= respText.size()) {
            statusCode = std::atoi(respText.substr(firstSp + 1, 3).c_str());
        }
        const auto hdrEnd = respText.find("\r\n\r\n");
        std::string body = (hdrEnd == std::string::npos)
            ? respText
            : respText.substr(hdrEnd + 4);

        AsyncNetworkClient::RestResponse out;
        out.statusCode = statusCode;
        out.body = std::move(body);
        out.recvTimestampNs = AsyncNetworkClient::nowNs();
        out.errorClass = AsyncNetworkClient::classifyHttpStatus(statusCode, req.signedRequest);
        if (out.errorClass != AsyncNetworkClient::RestErrorClass::NONE) {
            out.error = "HTTP status " + std::to_string(statusCode);
        }
        if (statusCode == 0) {
            out.error = "HTTP status parse failed";
            out.errorClass = AsyncNetworkClient::RestErrorClass::PARSE;
        }

        if (cb) {
            cb(out);
        }
    }).detach();

    return true;
}

void AsyncNetworkClient::setWsMessageCallback(WsMessageCallback cb) noexcept
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_wsCallback = std::move(cb);
}

void AsyncNetworkClient::setDisconnectCallback(DisconnectCallback cb) noexcept
{
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_disconnectCallback = std::move(cb);
}

void AsyncNetworkClient::setHeartbeatPolicy(uint32_t pingIntervalMs,
                                            uint32_t pongTimeoutMs) noexcept
{
    m_heartbeatPingIntervalMs.store(std::max<uint32_t>(100, pingIntervalMs));
    m_heartbeatPongTimeoutMs.store(std::max<uint32_t>(500, pongTimeoutMs));
}

void AsyncNetworkClient::setReconnectPolicy(bool enabled,
                                            uint32_t initialBackoffMs,
                                            uint32_t maxBackoffMs,
                                            uint32_t maxAttempts) noexcept
{
    m_reconnectEnabled.store(enabled);
    m_reconnectInitialBackoffMs.store(std::max<uint32_t>(50, initialBackoffMs));
    m_reconnectMaxBackoffMs.store(std::max(m_reconnectInitialBackoffMs.load(),
                                           std::max<uint32_t>(100, maxBackoffMs)));
    m_reconnectMaxAttempts.store(maxAttempts);
    m_reconnectCurrentBackoffMs.store(m_reconnectInitialBackoffMs.load());
}

void AsyncNetworkClient::setStrictTlsValidation(bool enabled,
                                                std::string caCertPath,
                                                std::string expectedServerName,
                                                bool intranetLoopbackOnly) noexcept
{
    m_tlsStrictValidation.store(enabled);
    m_tlsIntranetLoopbackOnly.store(intranetLoopbackOnly);
    std::lock_guard<std::mutex> lock(m_tlsConfigMutex);
    m_tlsCaCertPath = std::move(caCertPath);
    m_tlsExpectedServerName = std::move(expectedServerName);
}

bool AsyncNetworkClient::isLoopbackHost(std::string_view host) noexcept
{
    if (host == "localhost" || host == "127.0.0.1" || host == "::1") {
        return true;
    }
    if (host.size() >= 4 && host.substr(0, 4) == "127.") {
        return true;
    }
    return false;
}

void AsyncNetworkClient::notifyLoop() noexcept
{
#if defined(__linux__)
    if (m_eventFd >= 0) {
        const uint64_t one = 1;
        const ssize_t wrote = ::write(m_eventFd, &one, sizeof(one));
        (void)wrote;
    }
#else
    if (m_wakeWriteFd >= 0) {
        const char one = 1;
        const ssize_t wrote = ::write(m_wakeWriteFd, &one, sizeof(one));
        (void)wrote;
    }
#endif
}

void AsyncNetworkClient::injectTestWebSocketPayload(std::string_view payload) noexcept
{
    InjectedFrame frame;
    frame.kind = InjectedKind::PlainPayload;
    frame.size = static_cast<uint16_t>(
        std::min<std::size_t>(frame.data.size() - 1, payload.size()));
    std::memcpy(frame.data.data(), payload.data(), frame.size);
    frame.data[frame.size] = '\0';

    if (m_injectedFrames.push(frame)) {
        notifyLoop();
    }
}

void AsyncNetworkClient::injectTestWebSocketFrame(std::string_view frameBytes) noexcept
{
    InjectedFrame frame;
    frame.kind = InjectedKind::RawFrame;
    frame.size = static_cast<uint16_t>(
        std::min<std::size_t>(frame.data.size(), frameBytes.size()));
    if (frame.size > 0) {
        std::memcpy(frame.data.data(), frameBytes.data(), frame.size);
    }

    if (m_injectedFrames.push(frame)) {
        notifyLoop();
    }
}

void AsyncNetworkClient::injectSecureTestWebSocketPayload(std::string_view payload) noexcept
{
    const std::string rawFrame = buildWsFrame(WS_OPCODE_TEXT, payload, false);
    const std::string secure = xorSecureEnvelope(rawFrame);

    InjectedFrame frame;
    frame.kind = InjectedKind::SecureFrame;
    frame.size = static_cast<uint16_t>(
        std::min<std::size_t>(frame.data.size(), secure.size()));
    if (frame.size > 0) {
        std::memcpy(frame.data.data(), secure.data(), frame.size);
    }

    if (m_injectedFrames.push(frame)) {
        notifyLoop();
    }
}

void AsyncNetworkClient::consumeInjectedFrame(const InjectedFrame& frame)
{
    if (frame.kind == InjectedKind::PlainPayload) {
        WsMessageCallback cb;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            cb = m_wsCallback;
        }
        const uint64_t recvNs = nowNs();
        if (cb) {
            cb(std::string_view(frame.data.data(), frame.size), recvNs);
        }
        m_lastWsRecvNs.store(recvNs);
        m_wsMessagesReceived.fetch_add(1);
        m_wsBytesReceived.fetch_add(frame.size);
        return;
    }

    std::string raw(frame.data.data(), frame.size);
    if (frame.kind == InjectedKind::SecureFrame) {
        raw = xorSecureEnvelope(raw);
    }

    {
        std::lock_guard<std::mutex> lock(m_wsMutex);
        m_wsRxBuffer.insert(m_wsRxBuffer.end(), raw.begin(), raw.end());
    }
    processWsRxBuffer();
}

void AsyncNetworkClient::onSocketReadable()
{
    std::array<char, 4096> buf{};
    while (true) {
        int fd = -1;
        bool tls = false;
        void* ssl = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_wsMutex);
            fd = m_wsFd;
            tls = m_wsUsesTls;
            ssl = m_wsSsl;
        }
        if (fd < 0) {
            break;
        }

        std::string err;
        ssize_t n = 0;
        if (tls) {
            n = OpenSslShim::instance().read(ssl, buf.data(), buf.size(), true, err);
        } else {
            n = ::recv(fd, buf.data(), buf.size(), 0);
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) {
                n = 0;
            } else if (n < 0) {
                err = "recv failed: " + std::string(std::strerror(errno));
            }
        }

        if (n > 0) {
            {
                std::lock_guard<std::mutex> lock(m_wsMutex);
                m_wsRxBuffer.insert(m_wsRxBuffer.end(), buf.begin(), buf.begin() + n);
            }
            m_lastWsRecvNs.store(nowNs());
            processWsRxBuffer();
            continue;
        }
        if (n == 0) {
            break;
        }
        setLastError(err.empty() ? "socket read failed" : err);
        handleDisconnect();
        break;
    }
}

std::string AsyncNetworkClient::buildWsFrame(uint8_t opcode,
                                             std::string_view payload,
                                             bool maskClientFrame)
{
    std::string out;
    out.reserve(payload.size() + 16);
    out.push_back(static_cast<char>(0x80u | (opcode & 0x0fu)));

    const std::size_t len = payload.size();
    uint8_t lenByte = 0;
    if (len <= 125) {
        lenByte = static_cast<uint8_t>(len);
        out.push_back(static_cast<char>((maskClientFrame ? 0x80u : 0u) | lenByte));
    } else if (len <= 0xffff) {
        out.push_back(static_cast<char>((maskClientFrame ? 0x80u : 0u) | 126u));
        out.push_back(static_cast<char>((len >> 8) & 0xff));
        out.push_back(static_cast<char>(len & 0xff));
    } else {
        out.push_back(static_cast<char>((maskClientFrame ? 0x80u : 0u) | 127u));
        for (int i = 7; i >= 0; --i) {
            out.push_back(static_cast<char>((len >> (i * 8)) & 0xff));
        }
    }

    if (maskClientFrame) {
        const uint32_t seed = static_cast<uint32_t>(nowNs() & 0xffffffffu);
        const std::array<uint8_t, 4> mask{
            static_cast<uint8_t>((seed >> 24) & 0xff),
            static_cast<uint8_t>((seed >> 16) & 0xff),
            static_cast<uint8_t>((seed >> 8) & 0xff),
            static_cast<uint8_t>(seed & 0xff)
        };
        for (uint8_t v : mask) {
            out.push_back(static_cast<char>(v));
        }
        for (std::size_t i = 0; i < payload.size(); ++i) {
            out.push_back(static_cast<char>(
                static_cast<uint8_t>(payload[i]) ^ mask[i % 4]));
        }
    } else {
        out.append(payload.data(), payload.size());
    }

    return out;
}

std::string AsyncNetworkClient::xorSecureEnvelope(std::string_view data)
{
    constexpr uint8_t k = 0xA5u;
    std::string out(data);
    for (char& c : out) {
        c = static_cast<char>(static_cast<uint8_t>(c) ^ k);
    }
    return out;
}

bool AsyncNetworkClient::sendWsFrame(uint8_t opcode,
                                     std::string_view payload,
                                     bool maskClientFrame)
{
    std::string err;
    const std::string frame = buildWsFrame(opcode, payload, maskClientFrame);

    int fd = -1;
    bool tls = false;
    void* ssl = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_wsMutex);
        fd = m_wsFd;
        tls = m_wsUsesTls;
        ssl = m_wsSsl;
    }
    if (fd < 0) {
        return false;
    }

    bool ok = false;
    if (tls) {
        ok = tlsSendAll(ssl, frame, err);
    } else {
        ok = plainSendAll(fd, frame, err);
    }
    if (!ok) {
        setLastError(err.empty() ? "ws frame send failed" : err);
    }
    return ok;
}

void AsyncNetworkClient::processWsRxBuffer()
{
    struct DecodedFrame {
        uint8_t opcode{0};
        bool fin{false};
        std::string payload;
    };

    auto popOneFrame = [this]() -> std::optional<DecodedFrame> {
        std::lock_guard<std::mutex> lock(m_wsMutex);
        if (m_wsRxBuffer.size() < 2) {
            return std::nullopt;
        }

        std::size_t idx = 0;
        const uint8_t b0 = static_cast<uint8_t>(m_wsRxBuffer[idx++]);
        const uint8_t b1 = static_cast<uint8_t>(m_wsRxBuffer[idx++]);
        const bool fin = (b0 & 0x80u) != 0;
        const uint8_t opcode = static_cast<uint8_t>(b0 & 0x0fu);
        const bool masked = (b1 & 0x80u) != 0;

        uint64_t payloadLen = static_cast<uint64_t>(b1 & 0x7fu);
        if (payloadLen == 126u) {
            if (m_wsRxBuffer.size() < idx + 2) {
                return std::nullopt;
            }
            payloadLen = (static_cast<uint64_t>(static_cast<uint8_t>(m_wsRxBuffer[idx])) << 8)
                       | static_cast<uint64_t>(static_cast<uint8_t>(m_wsRxBuffer[idx + 1]));
            idx += 2;
        } else if (payloadLen == 127u) {
            if (m_wsRxBuffer.size() < idx + 8) {
                return std::nullopt;
            }
            payloadLen = 0;
            for (int i = 0; i < 8; ++i) {
                payloadLen = (payloadLen << 8)
                    | static_cast<uint64_t>(static_cast<uint8_t>(m_wsRxBuffer[idx + static_cast<std::size_t>(i)]));
            }
            idx += 8;
        }

        std::array<uint8_t, 4> maskKey{};
        if (masked) {
            if (m_wsRxBuffer.size() < idx + 4) {
                return std::nullopt;
            }
            for (int i = 0; i < 4; ++i) {
                maskKey[static_cast<std::size_t>(i)] =
                    static_cast<uint8_t>(m_wsRxBuffer[idx + static_cast<std::size_t>(i)]);
            }
            idx += 4;
        }

        if (payloadLen > (1ULL << 20)) {
            m_wsRxBuffer.clear();
            return DecodedFrame{WS_OPCODE_CLOSE, true, {}};
        }
        if (m_wsRxBuffer.size() < idx + static_cast<std::size_t>(payloadLen)) {
            return std::nullopt;
        }

        DecodedFrame frame;
        frame.opcode = opcode;
        frame.fin = fin;
        frame.payload.resize(static_cast<std::size_t>(payloadLen));
        for (std::size_t i = 0; i < frame.payload.size(); ++i) {
            uint8_t v = static_cast<uint8_t>(m_wsRxBuffer[idx + i]);
            if (masked) {
                v ^= maskKey[i % 4];
            }
            frame.payload[i] = static_cast<char>(v);
        }

        m_wsRxBuffer.erase(m_wsRxBuffer.begin(),
                           m_wsRxBuffer.begin() + static_cast<long>(idx + frame.payload.size()));
        return frame;
    };

    while (true) {
        auto frameOpt = popOneFrame();
        if (!frameOpt.has_value()) {
            break;
        }
        const DecodedFrame frame = std::move(*frameOpt);

        if (frame.opcode == WS_OPCODE_PING) {
            (void)sendWsFrame(WS_OPCODE_PONG, frame.payload, true);
            continue;
        }
        if (frame.opcode == WS_OPCODE_PONG) {
            m_wsPongsReceived.fetch_add(1);
            m_lastWsPongNs.store(nowNs());
            continue;
        }
        if (frame.opcode == WS_OPCODE_CLOSE) {
            handleDisconnect();
            break;
        }

        if (frame.opcode == WS_OPCODE_CONTINUATION) {
            m_wsFragmentBuffer.insert(m_wsFragmentBuffer.end(),
                                      frame.payload.begin(), frame.payload.end());
            if (frame.fin) {
                WsMessageCallback cb;
                {
                    std::lock_guard<std::mutex> lock(m_callbackMutex);
                    cb = m_wsCallback;
                }
                const uint64_t recvNs = nowNs();
                if (cb && !m_wsFragmentBuffer.empty()) {
                    cb(std::string_view(m_wsFragmentBuffer.data(), m_wsFragmentBuffer.size()), recvNs);
                }
                m_lastWsRecvNs.store(recvNs);
                m_wsMessagesReceived.fetch_add(1);
                m_wsBytesReceived.fetch_add(m_wsFragmentBuffer.size());
                m_wsFragmentBuffer.clear();
                m_wsFragmentOpcode = 0;
            }
            continue;
        }

        if ((frame.opcode == WS_OPCODE_TEXT || frame.opcode == WS_OPCODE_BINARY) && !frame.fin) {
            m_wsFragmentBuffer.assign(frame.payload.begin(), frame.payload.end());
            m_wsFragmentOpcode = frame.opcode;
            continue;
        }

        if (frame.opcode == WS_OPCODE_TEXT || frame.opcode == WS_OPCODE_BINARY) {
            WsMessageCallback cb;
            {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                cb = m_wsCallback;
            }
            const uint64_t recvNs = nowNs();
            if (cb) {
                cb(frame.payload, recvNs);
            }
            m_lastWsRecvNs.store(recvNs);
            m_wsMessagesReceived.fetch_add(1);
            m_wsBytesReceived.fetch_add(frame.payload.size());
        }
    }
}

void AsyncNetworkClient::handleDisconnect()
{
    const bool wasConnected = m_wsConnected.exchange(false);
    closeWsConnection();
    if (!wasConnected) {
        return;
    }

    DisconnectCallback dcb;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        dcb = m_disconnectCallback;
    }
    if (dcb) {
        dcb();
    }

    m_wsReconnectAttempts.fetch_add(1);
    scheduleReconnect();
}

void AsyncNetworkClient::updateHeartbeat()
{
    if (!m_wsConnected.load()) {
        const uint64_t nextReconnect = m_nextReconnectNs.load();
        if (nextReconnect != 0 && nowNs() >= nextReconnect && !m_wsUrl.empty()) {
            m_nextReconnectNs.store(0);
            if (!connectWebSocketInternal(m_wsParsedUrl)) {
                m_wsReconnectAttempts.fetch_add(1);
                scheduleReconnect();
            }
        }
        return;
    }

    if (m_wsParsedUrl.scheme == "mock") {
        return;
    }

    const uint64_t now = nowNs();
    const uint64_t pingIntervalNs =
        static_cast<uint64_t>(m_heartbeatPingIntervalMs.load()) * 1'000'000ULL;
    const uint64_t pongTimeoutNs =
        static_cast<uint64_t>(m_heartbeatPongTimeoutMs.load()) * 1'000'000ULL;

    const uint64_t lastPing = m_lastWsPingNs.load();
    if (lastPing == 0 || (now - lastPing) >= pingIntervalNs) {
        if (sendWsFrame(WS_OPCODE_PING, {}, true)) {
            m_lastWsPingNs.store(now);
            m_wsPingsSent.fetch_add(1);
        }
    }

    const uint64_t lastPong = m_lastWsPongNs.load();
    const uint64_t reference = std::max(lastPong, m_lastWsRecvNs.load());
    if (reference != 0 && (now - reference) > pongTimeoutNs) {
        setLastError("Heartbeat timeout waiting for pong");
        handleDisconnect();
    }
}

void AsyncNetworkClient::runLoop()
{
#if defined(__linux__)
    std::array<epoll_event, 16> events{};
    while (!m_stopRequested.load()) {
        const int n = ::epoll_wait(m_epollFd, events.data(),
                                   static_cast<int>(events.size()), 100);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == m_eventFd) {
                uint64_t counter = 0;
                const ssize_t readN = ::read(m_eventFd, &counter, sizeof(counter));
                (void)readN;

                InjectedFrame frame;
                while (m_injectedFrames.pop(frame)) {
                    consumeInjectedFrame(frame);
                }
                continue;
            }

            int wsFdSnapshot = -1;
            {
                std::lock_guard<std::mutex> lock(m_wsMutex);
                wsFdSnapshot = m_wsFd;
            }
            if (events[i].data.fd == wsFdSnapshot) {
                if ((events[i].events & (EPOLLERR | EPOLLRDHUP | EPOLLHUP)) != 0) {
                    handleDisconnect();
                    continue;
                }
                if ((events[i].events & EPOLLIN) != 0) {
                    onSocketReadable();
                }
            }
        }

        updateHeartbeat();
    }
#else
    while (!m_stopRequested.load()) {
        std::array<pollfd, 2> fds{};
        nfds_t count = 0;
        if (m_eventFd >= 0) {
            fds[count].fd = m_eventFd;
            fds[count].events = POLLIN;
            ++count;
        }

        int wsFdSnapshot = -1;
        {
            std::lock_guard<std::mutex> lock(m_wsMutex);
            wsFdSnapshot = m_wsFd;
        }
        if (wsFdSnapshot >= 0 && count < fds.size()) {
            fds[count].fd = wsFdSnapshot;
            fds[count].events = POLLIN;
            ++count;
        }

        const int n = ::poll(fds.data(), count, 100);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        for (nfds_t i = 0; i < count; ++i) {
            if (fds[i].revents == 0) {
                continue;
            }
            if (fds[i].fd == m_eventFd) {
                char buffer[64];
                while (::read(m_eventFd, buffer, sizeof(buffer)) > 0) {}

                InjectedFrame frame;
                while (m_injectedFrames.pop(frame)) {
                    consumeInjectedFrame(frame);
                }
                continue;
            }

            if (fds[i].fd == wsFdSnapshot) {
                if ((fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
                    handleDisconnect();
                    continue;
                }
                if ((fds[i].revents & POLLIN) != 0) {
                    onSocketReadable();
                }
            }
        }

        updateHeartbeat();
    }
#endif
}

void AsyncNetworkClient::setLastError(const std::string& err)
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_lastError = err;
}

uint64_t AsyncNetworkClient::nowNs() noexcept
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
}

bool AsyncNetworkClient::isRunning() const noexcept
{
    return m_running.load();
}

bool AsyncNetworkClient::isWsConnected() const noexcept
{
    return m_wsConnected.load();
}

uint64_t AsyncNetworkClient::wsMessagesReceived() const noexcept
{
    return m_wsMessagesReceived.load();
}

uint64_t AsyncNetworkClient::wsBytesReceived() const noexcept
{
    return m_wsBytesReceived.load();
}

uint64_t AsyncNetworkClient::wsPingsSent() const noexcept
{
    return m_wsPingsSent.load();
}

uint64_t AsyncNetworkClient::wsPongsReceived() const noexcept
{
    return m_wsPongsReceived.load();
}

uint64_t AsyncNetworkClient::wsReconnectAttempts() const noexcept
{
    return m_wsReconnectAttempts.load();
}

uint64_t AsyncNetworkClient::restRequestsIssued() const noexcept
{
    return m_restRequestsIssued.load();
}

std::string AsyncNetworkClient::lastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}
