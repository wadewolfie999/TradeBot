#include "ExecutionEngine.hpp"
#include "L2OrderBook.hpp"
#include "LocalDataReplayAdapter.hpp"
#include "PortfolioManager.hpp"
#include "RiskEngine.hpp"
#include "TriggerOrderManager.hpp"

#include <filesystem>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool cond, const char* msg)
{
    if (!cond) {
        throw std::runtime_error(msg);
    }
}

void test_local_replay_csv_and_binary_roundtrip()
{
    const std::filesystem::path base = std::filesystem::path("/tmp") / "aiio_phase18_tests";
    std::filesystem::create_directories(base);
    const auto csvPath = (base / "replay_ticks.csv").string();
    const auto binPath = (base / "replay_ticks.bin").string();

    {
        FILE* fp = std::fopen(csvPath.c_str(), "w");
        require(fp != nullptr, "failed to open temporary CSV path");
        std::fprintf(fp, "timestamp_ns,bid_px,bid_qty,ask_px,ask_qty,trade_px,trade_qty,symbol\n");
        std::fprintf(fp, "1710000000000000000,100.0,1.2,100.1,1.1,100.05,0.4,BTCUSDT\n");
        std::fprintf(fp, "1710000000001000000,100.1,1.1,100.2,1.0,100.15,0.6,BTCUSDT\n");
        std::fclose(fp);
    }

    LocalDataReplayAdapter replay;
    require(replay.loadCsv(csvPath, "BTCUSDT"), "CSV replay load failed");
    require(replay.size() == 2, "CSV replay size mismatch");

    std::vector<ReplayTick> ticks;
    ReplayTick tick;
    while (replay.nextTick(tick)) {
        ticks.push_back(tick);
    }
    require(ticks.size() == 2, "CSV replay iteration mismatch");

    std::string writeErr;
    require(LocalDataReplayAdapter::writeBinary(binPath, ticks, &writeErr),
            "binary replay write failed");

    LocalDataReplayAdapter replayBin;
    require(replayBin.loadBinary(binPath, "BTCUSDT"), "binary replay load failed");
    require(replayBin.size() == ticks.size(), "binary replay size mismatch");

    ReplayTick fromBin;
    require(replayBin.nextTick(fromBin), "binary replay first tick missing");
    require(fromBin.bidPrice > 0.0 && fromBin.askPrice > fromBin.bidPrice,
            "binary replay top-of-book invalid");
}

void test_l2_orderbook_o1_bbo_updates()
{
    L2OrderBook book(0.01, 1024);
    require(book.applyBbo(100.00, 2.0, 100.02, 1.5), "initial BBO apply failed");

    auto bbo = book.bbo();
    require(bbo.valid, "BBO expected valid after first apply");
    require(bbo.bidPrice > 0.0 && bbo.askPrice > bbo.bidPrice, "BBO shape invalid");

    require(book.applyBidLevel(100.01, 3.0), "bid improvement failed");
    bbo = book.bbo();
    require(bbo.bidPrice > 100.005, "best bid did not move in O(1) path");

    require(book.applyAskLevel(100.01, 0.0), "ask deletion failed");
    require(book.applyAskLevel(100.03, 2.5), "ask replacement failed");
    bbo = book.bbo();
    require(bbo.askPrice > 100.015, "best ask invalid after level mutation");
}

void test_stoploss_and_oco_trigger_execution_path()
{
    PortfolioManager portfolio;
    RiskEngine riskEngine(portfolio, 1, 0.0);
    ExecutionEngine execution(portfolio, riskEngine, "BTCUSDT", 0.001, 5.0, 0.01);

    TriggerOrderManager triggers(128);
    execution.bindTriggerOrderManager(&triggers);

    L2OrderBook book(0.01, 8192);

    const bool opened = execution.execute(Signal::BUY, 100.0, 1, "PHASE18_TEST");
    require(opened, "failed to open initial long position for trigger test");
    require(portfolio.hasPosition("BTCUSDT"), "expected open position after entry");

    const double entry = portfolio.getEntryPrice("BTCUSDT");
    const uint64_t ocoId = execution.placeOcoTrigger(entry * 0.995, entry * 1.002, 0.0, "OCO_TEST");
    require(ocoId != 0, "failed to place OCO trigger");

    require(book.applyBbo(entry * 1.003, 1.0, entry * 1.004, 1.0), "failed to apply bullish BBO");
    const int triggered = execution.processTriggerOrders(book, 2);
    require(triggered >= 1, "expected OCO take-profit trigger to fire");
    require(!portfolio.hasPosition("BTCUSDT"), "position should be closed after OCO trigger");

    const bool reopened = execution.execute(Signal::BUY, 100.0, 3, "PHASE18_TEST");
    require(reopened, "failed to reopen position for stop-loss branch");
    const uint64_t stopId = execution.placeStopLossTrigger(99.0, 0.0, "STOP_TEST");
    require(stopId != 0, "failed to place stop-loss trigger");

    require(book.applyBbo(98.9, 1.0, 99.0, 1.0), "failed to apply bearish BBO");
    const int stopTriggered = execution.processTriggerOrders(book, 4);
    require(stopTriggered >= 1, "expected stop-loss trigger to fire");
    require(!portfolio.hasPosition("BTCUSDT"), "position should be closed after stop-loss trigger");
}

} // namespace

int main()
{
    test_local_replay_csv_and_binary_roundtrip();
    test_l2_orderbook_o1_bbo_updates();
    test_stoploss_and_oco_trigger_execution_path();
    return 0;
}
