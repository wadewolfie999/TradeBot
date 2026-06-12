#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <cmath>
#include <numeric>
#include <vector>
#include <queue>
#include <functional>
#include <algorithm>
#include <memory>
#include <thread>
#include <unordered_map>

#include "CsvReader.hpp"
#include "EventLoop.hpp"
#include "RiskEngine.hpp"
#include "SystemConfig.hpp"
#include "LiveDataAdapter.hpp"
#include "BrokerGateway.hpp"
#include "SmaCrossStrategy.hpp"
#include "MeanReversionStrategy.hpp"
#include "PortfolioAllocator.hpp"
#include "PortfolioManager.hpp"
#include "ExecutionEngine.hpp"
#include "AnalyticsEngine.hpp"
#include "StateSerializer.hpp"
#include "RegimeDetector.hpp"

// ─── Metrics helpers ─────────────────────────────────────────────────────────

// Compute Sharpe Ratio from equity snapshots:
//   per-candle returns → mean / stddev (annualised by sqrt of bars per year).
// 15-min bars → 365 * 24 * 4 = 35040 bars/year.
static double computeSharpe(const std::vector<EquityPoint>& equity,
                             double barsPerYear = 35040.0)
{
    if (equity.size() < 2) { return 0.0; }

    std::vector<double> returns;
    returns.reserve(equity.size() - 1);
    for (std::size_t i = 1; i < equity.size(); ++i) {
        if (equity[i - 1].equity > 0.0) {
            returns.push_back((equity[i].equity - equity[i - 1].equity)
                              / equity[i - 1].equity);
        }
    }
    if (returns.empty()) { return 0.0; }

    const double mean = std::accumulate(returns.begin(), returns.end(), 0.0)
                        / static_cast<double>(returns.size());

    double variance = 0.0;
    for (const double r : returns) {
        variance += (r - mean) * (r - mean);
    }
    variance /= static_cast<double>(returns.size());
    const double stddev = std::sqrt(variance);

    if (stddev == 0.0) { return 0.0; }
    return (mean / stddev) * std::sqrt(barsPerYear);
}

struct TradeMetrics {
    double profitFactor{0.0};
    double winRate{0.0};
    double expectancy{0.0};
    int    winners{0};
    int    losers{0};
    double grossProfit{0.0};
    double grossLoss{0.0};
};

static TradeMetrics computeTradeMetrics(const std::vector<TradeRecord>& log)
{
    TradeMetrics m;
    if (log.empty()) { return m; }

    for (const auto& t : log) {
        if (t.realizedPnL >= 0.0) {
            ++m.winners;
            m.grossProfit += t.realizedPnL;
        } else {
            ++m.losers;
            m.grossLoss += std::abs(t.realizedPnL);
        }
    }

    const int total = static_cast<int>(log.size());
    m.winRate      = total > 0 ? (static_cast<double>(m.winners) / total) * 100.0 : 0.0;
    m.profitFactor = m.grossLoss > 0.0 ? m.grossProfit / m.grossLoss : 0.0;

    double totalPnL = 0.0;
    for (const auto& t : log) { totalPnL += t.realizedPnL; }
    m.expectancy = total > 0 ? totalPnL / total : 0.0;

    return m;
}

// ─── Multi-stream chronological multiplexer ───────────────────────────────────

struct StreamEntry {
    MarketCandle  candle;
    CsvReader*    reader{nullptr};

    bool operator>(const StreamEntry& other) const noexcept {
        return candle.epochTimestamp > other.candle.epochTimestamp;
    }
};

// ─── main ────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    // ── Phase 12: System mode + Phase 9 --resume flag ────────────────────
    SystemConfig sysCfg;  // defaults to BACKTEST
    std::string resumeFile;
    bool        doResume = false;

    struct StreamSpec {
        std::string filepath;
        std::string symbol;
    };

    std::vector<StreamSpec> specs;

    if (argc >= 2) {
        int i = 1;
        for (; i < argc; ++i) {
            if (std::strcmp(argv[i], "--resume") == 0) {
                if (i + 1 >= argc) {
                    std::cerr << "[Error] --resume requires a snapshot file argument.\n";
                    return EXIT_FAILURE;
                }
                resumeFile = argv[i + 1];
                doResume   = true;
                ++i;
            } else if (std::strcmp(argv[i], "--mode") == 0) {
                if (i + 1 >= argc) {
                    std::cerr << "[Error] --mode requires an argument: backtest|paper|live\n";
                    return EXIT_FAILURE;
                }
                sysCfg.mode = parseModeFlag(std::string(argv[i + 1]));
                ++i;
            } else {
                break;
            }
        }
        for (; i < argc; ++i) {
            std::string path = argv[i];
            std::string stem = path;
            const auto slash = stem.find_last_of("/\\");
            if (slash != std::string::npos) { stem = stem.substr(slash + 1); }
            const auto dot = stem.rfind('.');
            if (dot != std::string::npos) { stem = stem.substr(0, dot); }
            specs.push_back({path, stem});
        }
    } else {
        specs.push_back({"data/BTCUSDT-15.csv", "BTCUSDT-15"});
    }

    // If no file specs were collected (e.g. only --mode / --resume flags were
    // given with no positional file arguments), fall back to the default CSV.
    if (specs.empty()) {
        specs.push_back({"data/BTCUSDT-15.csv", "BTCUSDT-15"});
    }

    // ── Open all CSV streams ──────────────────────────────────────────────
    std::vector<std::unique_ptr<CsvReader>> readers;
    readers.reserve(specs.size());
    for (const auto& spec : specs) {
        auto reader = std::make_unique<CsvReader>(spec.filepath, spec.symbol);
        if (!reader->isOpen()) {
            std::cerr << "[Error] Unable to open CSV file: " << spec.filepath << "\n";
            return EXIT_FAILURE;
        }
        readers.push_back(std::move(reader));
    }

    // ── Engine setup ──────────────────────────────────────────────────────
    PortfolioManager portfolio;
    AnalyticsEngine  analytics;
    StateSerializer  serializer;

    // MaxConcurrentPositions = 2: allow at most 2 open positions simultaneously.
    RiskEngine riskEngine(portfolio, 2);
    // Phase 12: inject SystemConfig so circuit-breaker thresholds are configurable.
    riskEngine.setSystemConfig(&sysCfg);

    // ── Phase 12: Live data adapter + broker gateway ───────────────────────
    // Both are constructed regardless of mode; in BACKTEST they are no-ops.
    LiveDataAdapter liveAdapter(sysCfg);
    BrokerGateway   brokerGateway(sysCfg, portfolio);
    std::unordered_map<std::string, MarketRegime> lastRegimeBySymbol;

    if (!sysCfg.isBacktest()) {
        liveAdapter.connect();
        brokerGateway.connect();
        // Wire fill callback: BrokerGateway notifies RiskEngine of API errors.
        brokerGateway.setFillCallback([&](const BrokerFill& fill) {
            if (!fill.success) { riskEngine.reportApiError(); }
        });
    }

    // ── Phase 10: Portfolio Allocator with strategy weights ───────────────
    // W_sma = 0.6, W_mr = 0.4 (SMA tends to perform better in trending markets).
    PortfolioAllocator allocator(/*longThreshold=*/0.3, /*shortThreshold=*/0.3);
    allocator.setWeight("SMA_01", 0.6);
    allocator.setWeight("MR_01",  0.4);

    // ── Phase 11: RegimeDetector + dynamic weighting ──────────────────────
    // One shared RegimeDetector drives regime classification for all symbols.
    RegimeDetector regimeDetector;  // default: ADX-14, var-window-20
    allocator.setRegimeDetector(&regimeDetector);

    // Wire AnalyticsEngine telemetry -> PortfolioAllocator Phi updates.
    analytics.setPortfolioAllocator(&allocator);

    if (!sysCfg.isBacktest()) {
        liveAdapter.setRegimePollIntervalMs(100);
        liveAdapter.setRegimeBatchCallback([&](const std::vector<MarketCandle>& ticks) {
            for (const auto& tick : ticks) {
                const MarketRegime regime = regimeDetector.update(tick.high, tick.low, tick.close);
                analytics.recordRegimeMatrix(tick.epochTimestamp,
                                             tick.symbol,
                                             RegimeDetector::regimeName(regime),
                                             regimeDetector.currentADX(),
                                             regimeDetector.currentVariance());
                const auto it = lastRegimeBySymbol.find(tick.symbol);
                if (it == lastRegimeBySymbol.end() || it->second != regime) {
                    lastRegimeBySymbol[tick.symbol] = regime;
                    std::thread([&allocator, regime] {
                        allocator.recomputeWeights(regime);
                    }).detach();
                }
            }
        });
        liveAdapter.setIntegrityCallback([&](bool integrityOk) {
            riskEngine.setHaltTrading(!integrityOk);
        });
    }

    std::cout << "[Allocator] Phase 11 dynamic weighting active (lambda_regime=0.7, lambda_perf=0.3)\n";
    std::cout << "[Allocator] Static seed weights: SMA_01=" << allocator.getWeight("SMA_01")
              << "  MR_01=" << allocator.getWeight("MR_01") << " (fallback when Phi=0)\n";

    // ── Per-symbol containers ─────────────────────────────────────────────
    struct SymbolEngines {
        std::unique_ptr<SmaCrossStrategy>      stratSma;
        std::unique_ptr<MeanReversionStrategy> stratMr;
        std::unique_ptr<ExecutionEngine>       execution;
        std::unique_ptr<EventLoop>             loop;
    };
    std::unordered_map<std::string, SymbolEngines> engines;

    for (const auto& spec : specs) {
        SymbolEngines se;

        // Phase 10: instantiate both strategies with distinct IDs.
        se.stratSma = std::make_unique<SmaCrossStrategy>(12, 26, "SMA_01");
        se.stratMr  = std::make_unique<MeanReversionStrategy>(20, 2.0, 14, "MR_01");

        se.execution = std::make_unique<ExecutionEngine>(
                           portfolio, riskEngine, spec.symbol,
                           0.001,   // feeRate  = 0.1 %
                           5.0,     // slippage = 5 bps
                           0.01);   // riskPct  = 1 % of equity per trade

        // Phase 10: multi-strategy EventLoop driven by PortfolioAllocator.
        std::vector<IStrategy*> strategyList = {
            se.stratSma.get(),
            se.stratMr.get()
        };
        se.loop = std::make_unique<EventLoop>(
                      std::move(strategyList), allocator,
                      riskEngine, *se.execution, portfolio);

        se.execution->setAnalyticsEngine(&analytics);
        if (!sysCfg.isBacktest()) {
            se.execution->bindBrokerGateway(&brokerGateway);
        }
        // Wire ATR-based sizing from the SMA strategy (primary indicator).
        se.execution->setStrategy(se.stratSma.get());
        se.loop->setAnalyticsEngine(&analytics);
        // Phase 9: wire daily checkpointing (86400 s = 24 h).
        se.loop->setStateSerializer(&serializer, 86400);
        // Phase 11: drive regime updates through the shared RegimeDetector.
        se.loop->setRegimeDetector(&regimeDetector);

        engines[spec.symbol] = std::move(se);
    }

    std::cout << "[Multiplexer] Instance-per-Symbol engines: " << engines.size() << "\n";
    std::cout << "[RiskEngine]  MaxConcurrentPositions = "
              << riskEngine.getMaxConcurrentPositions() << "\n";
    std::cout << "[SystemMode]  " << modeName(sysCfg.mode)
              << "  (latency_max=" << sysCfg.latencyMaxMs << "ms"
              << ", error_thresh=" << sysCfg.errorRateThresh << "/min)\n";
    std::cout << "[Phase 11]    Strategies: SMA_01 + MR_01 via regime-aware PortfolioAllocator"
              << " (RegimeDetector: ADX-14, VarWindow-20)\n";

    // ── Phase 9: --resume state injection ─────────────────────────────────
    uint64_t resumeCheckpointTs = 0;
    if (doResume) {
        std::cout << "[Resume] Loading snapshot: " << resumeFile << "\n";
        if (!serializer.loadSnapshot(portfolio, riskEngine,
                                     regimeDetector, allocator,
                                     resumeCheckpointTs, resumeFile)) {
            std::cerr << "[Resume] ERROR: " << serializer.lastError() << "\n";
            return EXIT_FAILURE;
        }
        std::cout << "[Resume] Snapshot loaded. Checkpoint timestamp: "
                  << resumeCheckpointTs << "\n";
    }

    // ── Seed the priority queue with the first candle from every stream ───
    using MinHeap = std::priority_queue<StreamEntry,
                                        std::vector<StreamEntry>,
                                        std::greater<StreamEntry>>;
    MinHeap heap;

    for (auto& reader : readers) {
        StreamEntry entry;
        entry.reader = reader.get();
        if (entry.reader->readNextCandle(entry.candle)) {
            heap.push(std::move(entry));
        }
    }

    // ── Chronological dispatch loop ───────────────────────────────────────
    std::cout << "[Multiplexer] Active streams : " << readers.size() << "\n";

    while (!heap.empty()) {
        StreamEntry top = heap.top();
        heap.pop();

        MarketCandle routedCandle = top.candle;
        if (!sysCfg.isBacktest()) {
            liveAdapter.pushSimulatedCandle(top.candle);
            auto maybeLive = liveAdapter.getNextCandle(10);
            if (maybeLive) {
                routedCandle = *maybeLive;
            } else if (!liveAdapter.isConnected()) {
                // If disconnected and no candle available, skip until reconnect.
                StreamEntry next;
                next.reader = top.reader;
                if (next.reader->readNextCandle(next.candle)) {
                    heap.push(std::move(next));
                }
                continue;
            }
        }

        auto engIt = engines.find(routedCandle.symbol);
        if (engIt != engines.end()) {
            if (doResume && routedCandle.epochTimestamp <= resumeCheckpointTs) {
                // Warm up indicators without trading on resumed candles.
                engIt->second.stratSma->generateSignal(routedCandle);
                engIt->second.stratMr->generateSignal(routedCandle);
            } else {
                engIt->second.loop->processCandle(routedCandle);
            }
        } else {
            std::cerr << "[Multiplexer] WARNING: no engine for symbol '"
                      << routedCandle.symbol << "' — candle dropped.\n";
        }

        StreamEntry next;
        next.reader = top.reader;
        if (next.reader->readNextCandle(next.candle)) {
            heap.push(std::move(next));
        }
    }

    // ── Write CSV artefacts ───────────────────────────────────────────────
    const bool csvOk = analytics.finalize("data/results");
    if (csvOk) {
        std::cout << "\n[Analytics] CSV export successful:\n"
                  << "  data/results/trades.csv  (" << analytics.getTrades().size() << " rows)\n"
                  << "  data/results/equity.csv  (" << analytics.getEquity().size() << " rows)\n"
                  << "  data/results/slippage.csv  (" << analytics.getSlippage().size() << " rows)\n"
                  << "  data/results/regime_matrix.csv  (" << analytics.getRegimeMatrix().size() << " rows)\n";
    } else {
        std::cerr << "[Analytics] WARNING: CSV export failed.\n";
    }

    // ── Performance metrics ───────────────────────────────────────────────
    const double sharpe     = computeSharpe(analytics.getEquity());
    const TradeMetrics tm   = computeTradeMetrics(portfolio.getTradeLog());

    // ── Final backtest summary ────────────────────────────────────────────
    const double finalEquity  = portfolio.getTotalEquity();
    const double netPnL       = finalEquity - PortfolioManager::STARTING_CASH;
    const double netPnLPct    = (netPnL / PortfolioManager::STARTING_CASH) * 100.0;
    const double maxDD        = portfolio.getMaxDrawdown() * 100.0;
    const int    tradeCount   = portfolio.getTradeCount();
    const int    roundTrips   = portfolio.getRoundTripCount();
    const double totalFees    = portfolio.getTotalFeesPaid();
    const double finalVaR95   = riskEngine.getVaR95();
    const double finalStdDev  = riskEngine.getReturnStdDev();

    int signalCount  = 0;
    int blockedCount = 0;
    double maxRouteLatencyMs = 0.0;
    uint64_t routeLatencyBreaches = 0;
    for (const auto& [sym, se] : engines) {
        signalCount  += se.loop->getTotalSignals();
        blockedCount += se.loop->getRiskBlockedBuys()
                     +  se.execution->getBlockedCount();
        maxRouteLatencyMs = std::max(maxRouteLatencyMs, se.execution->getMaxRouteLatencyMs());
        routeLatencyBreaches += se.execution->getLatencyBreachCount();
    }

    // ── Per-strategy attribution from trade log ───────────────────────────
    std::unordered_map<std::string, TradeMetrics> stratMetrics;
    for (const auto& t : portfolio.getTradeLog()) {
        const std::string& sid = t.strategy_id.empty() ? "UNKNOWN" : t.strategy_id;
        TradeMetrics& sm = stratMetrics[sid];
        if (t.realizedPnL >= 0.0) {
            ++sm.winners;
            sm.grossProfit += t.realizedPnL;
        } else {
            ++sm.losers;
            sm.grossLoss += std::abs(t.realizedPnL);
        }
    }
    for (auto& [sid, sm] : stratMetrics) {
        const int tot  = sm.winners + sm.losers;
        sm.winRate      = tot > 0 ? (static_cast<double>(sm.winners) / tot) * 100.0 : 0.0;
        sm.profitFactor = sm.grossLoss > 0.0 ? sm.grossProfit / sm.grossLoss : 0.0;
        sm.expectancy   = tot > 0 ? (sm.grossProfit - sm.grossLoss) / tot : 0.0;
    }

    std::cout << "\n"
              << "====================================================\n"
              << "  EXECUTION SUMMARY  (fees=0.1%, slippage=5bps, risk=1%ATR)\n"
              << "  Phase 12: Live Execution Bridge & Adaptive Risk-On-The-Fly\n"
              << "  Mode: " << modeName(sysCfg.mode) << "\n"
              << "====================================================\n"
              << std::fixed << std::setprecision(2)
              << "  Starting Balance : $" << PortfolioManager::STARTING_CASH  << "\n"
              << "  Final Equity     : $" << finalEquity                       << "\n"
              << "  Net PnL          : $" << netPnL
              <<  " (" << netPnLPct << "%)\n"
              << "  Total Fees Paid  : $" << totalFees                         << "\n"
              << "  Max Drawdown     : "  << std::setprecision(4) << maxDD    << "%\n"
              << "  Final VaR(95%)   : $" << std::setprecision(2) << finalVaR95 << "\n"
              << "  Return StdDev    : "  << std::setprecision(6) << finalStdDev << "\n"
              << "  Signals Generated: "  << signalCount                       << "\n"
              << "  Trades Filled    : "  << tradeCount
              <<  " (" << roundTrips << " round-trips)\n"
              << "  Trades Blocked   : "  << blockedCount                      << "\n"
              << "  Max Route Latency: "  << std::setprecision(4) << maxRouteLatencyMs << " ms\n"
              << "  Latency Breaches : "  << routeLatencyBreaches << " (>5ms)\n"
              << "----------------------------------------------------\n"
              << "  PERFORMANCE METRICS\n"
              << "----------------------------------------------------\n"
              << std::setprecision(4)
              << "  Sharpe Ratio     : " << sharpe                             << "\n"
              << std::setprecision(4)
              << "  Profit Factor    : " << tm.profitFactor                    << "\n"
              << std::setprecision(2)
              << "  Win Rate         : " << tm.winRate                         << "%\n"
              << "  Expectancy/Trade : $" << tm.expectancy                     << "\n"
              << "  Winners          : " << tm.winners                         << "\n"
              << "  Losers           : " << tm.losers                          << "\n"
              << std::setprecision(2)
              << "  Gross Profit     : $" << tm.grossProfit                    << "\n"
              << "  Gross Loss       : $" << tm.grossLoss                      << "\n"
              << "----------------------------------------------------\n"
              << "  ATTRIBUTION BY STRATEGY\n"
              << "----------------------------------------------------\n";

    for (const auto& [sid, sm] : stratMetrics) {
        std::cout << "  [" << sid << "]\n"
                  << "    Win Rate     : " << std::setprecision(2) << sm.winRate << "%\n"
                  << "    Profit Factor: " << std::setprecision(4) << sm.profitFactor << "\n"
                  << "    Gross Profit : $" << std::setprecision(2) << sm.grossProfit << "\n"
                  << "    Gross Loss   : $" << sm.grossLoss << "\n"
                  << "    Winners      : " << sm.winners << "  Losers: " << sm.losers << "\n";
    }

    std::cout << "====================================================\n";
    std::cout << "----------------------------------------------------\n"
              << "  REGIME DETECTOR (final state)\n"
              << "----------------------------------------------------\n"
              << "  Final Regime     : " << RegimeDetector::regimeName(regimeDetector.currentRegime()) << "\n"
              << std::fixed << std::setprecision(4)
              << "  Final ADX        : " << regimeDetector.currentADX()      << "\n"
              << "  Final Variance   : " << regimeDetector.currentVariance() << "\n"
              << "----------------------------------------------------\n"
              << "  DYNAMIC WEIGHTS (Phi at end of run)\n"
              << "----------------------------------------------------\n"
              << "  Phi(SMA_01)      : " << allocator.getPhi("SMA_01") << "\n"
              << "  Phi(MR_01)       : " << allocator.getPhi("MR_01")  << "\n"
              << "====================================================\n";

    // ── Phase 12: Circuit-breaker final state ─────────────────────────────
    std::cout << "----------------------------------------------------\n"
              << "  CIRCUIT BREAKER STATE (Phase 12)\n"
              << "----------------------------------------------------\n"
              << "  Mode             : " << modeName(sysCfg.mode)                 << "\n"
              << "  CLOSE_ONLY       : " << (riskEngine.isCloseOnly() ? "YES" : "no") << "\n"
              << "  HALTED           : " << (riskEngine.isHalted()    ? "YES" : "no") << "\n"
              << "  Last Latency(ms) : " << riskEngine.lastLatencyMs()              << "\n"
              << "  Error Rate(/min) : " << riskEngine.currentErrorRate()           << "\n"
              << "====================================================\n";

    return EXIT_SUCCESS;
}
