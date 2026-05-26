#pragma once
// LiveDataAdapter.hpp -- WebSocket-driven real-time market data provider.
//
// Phase 12 (MOP-20260322-07, Workstream 7.2):
//   Implements the live-data half of the Data Engine.  In BACKTEST mode the
//   class behaves identically to CsvReader (sequential, deterministic).  In
//   PAPER or LIVE modes it:
//     - Maintains a persistent WSS connection to the configured endpoint.
//     - Parses incoming JSON tick messages into MarketCandle objects and
//       pushes them onto a thread-safe internal deque (the "candle queue").
//     - getNextCandle() blocks/yields until a candle is available.
//     - On disconnection, applies exponential back-off reconnection; once
//       reconnected, executes a REST gap-fill to recover any missed bars.
//
// Thread model
// ------------
//   A dedicated io-thread (simulated via a flag queue in this implementation)
//   feeds the candle queue.  The main trading thread calls getNextCandle()
//   which drains from the front of the queue.  A mutex guards the deque and
//   a condition_variable signals the consumer.
//
// Simulation harness
// ------------------
//   Because this codebase runs without a live network connection in the
//   development sandbox, the WSS connection is *simulated*: pushSimulatedCandle()
//   allows tests and the PAPER-mode harness to inject candles directly into
//   the queue, exercising all queue/backoff logic without a real socket.
#include "MarketCandle.hpp"
#include "AsyncNetworkClient.hpp"
#include "AuthManager.hpp"
#include "LockFreeRingBuffer.hpp"
#include "SystemConfig.hpp"
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional>
#include <string>
#include <string_view>
#include <cstdint>
#include <functional>
#include <chrono>
#include <vector>
#include <thread>

class LiveDataAdapter {
public:
    // Construct with a shared SystemConfig.  If cfg.isBacktest() is true,
    // the adapter operates in pass-through mode and getNextCandle() always
    // returns std::nullopt immediately (callers must use CsvReader in that case).
    explicit LiveDataAdapter(const SystemConfig& cfg) noexcept;

    ~LiveDataAdapter();

    // -- Connection lifecycle -----------------------------------------------

    // Begin the WSS connection (no-op in BACKTEST mode).
    // In PAPER/LIVE mode this simulates a successful initial connection and
    // starts the internal heartbeat timer.
    void connect();

    // Gracefully close the WSS connection and drain the pending candle queue.
    void disconnect() noexcept;

    // Returns true while the adapter considers itself connected.
    bool isConnected() const noexcept;

    // -- Candle queue interface ---------------------------------------------

    // Block until a candle is available (PAPER/LIVE) or return std::nullopt
    // if the adapter is disconnected and the queue is empty.
    // In BACKTEST mode always returns std::nullopt.
    // `timeoutMs` = 0 means wait indefinitely.
    std::optional<MarketCandle> getNextCandle(uint32_t timeoutMs = 0);

    // Non-blocking peek: returns the number of candles currently queued.
    std::size_t queueSize() const noexcept;

    // -- Simulation harness -------------------------------------------------

    // Inject a candle directly into the queue (for PAPER-mode harness / tests).
    // Thread-safe.
    void pushSimulatedCandle(const MarketCandle& candle);

    // -- Reconnection / gap-fill state (observable for tests) ---------------

    // Number of reconnection attempts since the last successful connect().
    uint32_t reconnectAttempts() const noexcept;

    // Number of REST gap-fill requests issued since construction.
    uint32_t gapFillCount() const noexcept;

    // -- Latency reporting (feeds RiskEngine circuit breaker) ---------------

    // Record a WSS ping/pong round-trip measurement in milliseconds.
    // Called by the I/O layer (or simulation harness) to update the rolling
    // latency tracking in RiskEngine.
    void recordLatencySample(uint32_t latencyMs) noexcept;

    // Latest recorded latency sample (ms).  Returns 0 if none recorded yet.
    uint32_t lastLatencyMs() const noexcept;

    // -- Event callbacks (optional) ----------------------------------------

    // Invoked on the I/O thread whenever a candle is pushed to the queue.
    // Use for logging / metrics.  Must be signal-safe (no allocation).
    using CandleCallback = std::function<void(const MarketCandle&)>;
    void setCandleCallback(CandleCallback cb) noexcept;

    // Invoked whenever a reconnection attempt is triggered, with the back-off
    // delay in milliseconds that will be applied before the next attempt.
    using ReconnectCallback = std::function<void(uint32_t /*attempt*/, uint32_t /*delayMs*/)>;
    void setReconnectCallback(ReconnectCallback cb) noexcept;

    using RegimeBatchCallback = std::function<void(const std::vector<MarketCandle>&)>;
    void setRegimeBatchCallback(RegimeBatchCallback cb) noexcept;
    void setRegimePollIntervalMs(uint32_t pollMs) noexcept;

    // Notifies data-integrity state transitions:
    // true -> integrity restored, false -> integrity degraded.
    using IntegrityCallback = std::function<void(bool)>;
    void setIntegrityCallback(IntegrityCallback cb) noexcept;

    // Phase 15 test hook: inject a raw external payload into the async client
    // callback path used by LIVE mode parsing/ring-buffer handoff.
    void injectExternalPayloadForTest(std::string_view payload) noexcept;
    void injectSecureExternalPayloadForTest(std::string_view payload) noexcept;

    // Observability for network->ring-buffer handoff latency.
    double networkHandoffP99Ms() const;

    // -- Internal simulation helpers (used by the PAPER-mode loop) ----------

    // Simulate a sudden disconnection (for circuit-breaker testing).
    void simulateDisconnect() noexcept;

    // Simulate a reconnection after back-off (bypasses real timers).
    void simulateReconnect();

    // Simulate a REST gap-fill completing with `count` synthetic candles.
    void simulateGapFill(uint32_t count);

private:
    struct TickNode;

    // Compute the current back-off delay and advance the attempt counter.
    uint32_t nextBackoffMs() noexcept;
    void startReconnectWorker();
    void stopReconnectWorker() noexcept;
    void pumpNetworkIngress();
    bool parseExternalTick(std::string_view payload, TickNode& out) noexcept;
    static uint64_t nowNs() noexcept;

    const SystemConfig& m_cfg;
    AsyncNetworkClient  m_networkClient;
    AuthManager         m_auth;

    struct TickNode {
        MarketCandle candle;
        bool         inUse{false};
        bool         networkOwned{false};
    };

    mutable std::mutex          m_mutex;
    std::condition_variable     m_cv;
    std::deque<TickNode*>       m_queue;
    std::vector<TickNode>       m_tickPool;
    std::vector<TickNode*>      m_freeNodes;
    uint32_t                    m_droppedTicks{0};
    uint32_t                    m_droppedNetworkTicks{0};
    std::thread                 m_reconnectWorker;
    LockFreeRingBuffer<TickNode*, 8192> m_networkIngress;
    LockFreeRingBuffer<TickNode*, 8192> m_networkFree;
    LockFreeRingBuffer<TickNode*, 8192> m_networkRecycle;
    std::vector<TickNode>       m_networkPool;
    std::vector<uint32_t>       m_networkHandoffUs;
    std::atomic<std::size_t>    m_networkHandoffCount{0};

    std::atomic<bool>    m_connected{false};
    std::atomic<bool>    m_shutdown{false};
    std::atomic<uint32_t> m_reconnectAttempts{0};
    std::atomic<uint32_t> m_gapFillCount{0};
    std::atomic<uint32_t> m_lastLatencyMs{0};

    uint32_t m_currentBackoffMs{0};
    uint32_t m_regimePollIntervalMs{100};
    std::vector<MarketCandle> m_pendingRegimeBatch;
    std::chrono::steady_clock::time_point m_lastRegimeFlush{
        std::chrono::steady_clock::now()
    };

    CandleCallback    m_candleCallback;
    ReconnectCallback m_reconnectCallback;
    RegimeBatchCallback m_regimeBatchCallback;
    IntegrityCallback m_integrityCallback;
};
