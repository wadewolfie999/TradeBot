#pragma once
// SystemConfig.hpp -- Centralized system-mode manager and runtime configuration.
//
// Phase 12 (MOP-20260322-07, Workstream 7.1):
//   Introduces a strongly-typed SystemMode enumeration and a SystemConfig
//   aggregate that is constructed once from CLI arguments and then injected
//   into DataEngine, ExecutionEngine, and RiskEngine so that each component
//   can branch on the operational mode without requiring a restart.
//
// Operational modes
// -----------------
//   BACKTEST  - fully deterministic, CSV-driven replay; no external network
//               calls are made.  Parity with previous phases is guaranteed.
//   PAPER     - routes data through LiveDataAdapter (WSS) but all broker
//               interactions are simulated locally inside BrokerGateway mock.
//   LIVE      - live-capable mode. Broker execution remains fail-closed unless
//               a separately approved adapter is injected into BrokerGateway.
//
// Circuit-breaker thresholds (Workstream 7.4)
// -------------------------------------------
//   latencyMaxMs    - WSS ping/pong or REST round-trip threshold in ms.
//                     Default: 500 ms.  Breach -> CLOSE_ONLY state.
//   errorRateThresh - Maximum tolerated API errors per minute.
//                     Default: 5.  Breach -> trading halted.
#include <string>
#include <cstdint>

// -- SystemMode --------------------------------------------------------------

enum class SystemMode : uint8_t {
    BACKTEST = 0,   // deterministic CSV replay (default)
    PAPER    = 1,   // live data, simulated execution
    LIVE     = 2    // live-capable; execution requires separate adapter approval
};

// Convert a CLI string (e.g. "paper") to SystemMode.
// Returns BACKTEST for unrecognised strings.
inline SystemMode parseModeFlag(const std::string& s) noexcept
{
    if (s == "PAPER"    || s == "paper")    { return SystemMode::PAPER;    }
    if (s == "LIVE"     || s == "live")     { return SystemMode::LIVE;     }
    if (s == "BACKTEST" || s == "backtest") { return SystemMode::BACKTEST; }
    return SystemMode::BACKTEST;
}

inline const char* modeName(SystemMode m) noexcept
{
    switch (m) {
        case SystemMode::BACKTEST: return "BACKTEST";
        case SystemMode::PAPER:    return "PAPER";
        case SystemMode::LIVE:     return "LIVE";
    }
    return "UNKNOWN";
}

// -- SystemConfig ------------------------------------------------------------

struct SystemConfig {
    // Operational mode -- set once at startup, read-only thereafter.
    SystemMode mode{SystemMode::BACKTEST};

    // -- Live adapter settings -----------------------------------------------
    std::string wssEndpoint{"wss://stream.example.com/ws"};
    std::string restEndpoint{"https://api.example.com"};
    std::string apiKey;
    std::string apiSecret;
    std::string apiKeyEnv{"AIIO_API_KEY"};
    std::string apiSecretEnv{"AIIO_API_SECRET"};
    bool strictTlsValidation{false};
    bool intranetLoopbackOnly{true};
    std::string localCaCertPath{"data/local_tls/aiio_local_ca.crt"};
    std::string tlsServerName{"localhost"};

    // Reconnection back-off: initial delay (ms), growth factor, cap (ms).
    uint32_t reconnectInitialMs{500};
    double   reconnectBackoffFactor{2.0};
    uint32_t reconnectMaxMs{30000};

    // -- Circuit-breaker thresholds (Workstream 7.4) -------------------------
    // Latency circuit breaker: if WSS round-trip > latencyMaxMs -> CLOSE_ONLY.
    uint32_t latencyMaxMs{500};

    // Error-rate circuit breaker: API errors per minute > errorRateThresh
    // -> halt all trading.
    uint32_t errorRateThresh{5};

    // -- Risk scaling factors ------------------------------------------------
    // ATR multiplier above which MaxConcurrentPositions is halved.
    double atrScaleUpThreshold{1.5};

    // VaR limit multipliers applied based on volatility regime.
    double varScaleLowVolFactor{1.0};   // normal conditions
    double varScaleHighVolFactor{0.5};  // high-volatility regime

    // -- Helpers -------------------------------------------------------------
    bool isLiveMode()  const noexcept { return mode == SystemMode::LIVE;     }
    bool isPaperMode() const noexcept { return mode == SystemMode::PAPER;    }
    bool isBacktest()  const noexcept { return mode == SystemMode::BACKTEST; }
};
