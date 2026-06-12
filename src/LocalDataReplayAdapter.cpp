#include "LocalDataReplayAdapter.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

namespace {

std::string trim(const std::string& s)
{
    const auto begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) { return {}; }
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

bool parseU64(const std::string& token, uint64_t& out)
{
    const std::string t = trim(token);
    if (t.empty()) { return false; }
    const char* begin = t.data();
    const char* end = t.data() + t.size();
    auto [ptr, ec] = std::from_chars(begin, end, out);
    return ec == std::errc{} && ptr == end;
}

bool parseDouble(const std::string& token, double& out)
{
    const std::string t = trim(token);
    if (t.empty()) { return false; }
    char* endPtr = nullptr;
    out = std::strtod(t.c_str(), &endPtr);
    return endPtr != nullptr && *endPtr == '\0' && std::isfinite(out);
}

std::vector<std::string> splitCsv(const std::string& line)
{
    std::vector<std::string> cols;
    cols.reserve(12);
    std::string cur;
    bool inQuote = false;

    for (char ch : line) {
        if (ch == '"') {
            inQuote = !inQuote;
            continue;
        }
        if (ch == ',' && !inQuote) {
            cols.push_back(cur);
            cur.clear();
            continue;
        }
        cur.push_back(ch);
    }
    cols.push_back(cur);
    return cols;
}

} // namespace

bool LocalDataReplayAdapter::loadFromPath(const std::string& path,
                                          const std::string& defaultSymbol)
{
    if (hasBinaryExtension(path)) {
        return loadBinary(path, defaultSymbol);
    }
    return loadCsv(path, defaultSymbol);
}

bool LocalDataReplayAdapter::loadCsv(const std::string& path,
                                     const std::string& defaultSymbol)
{
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        setLastError("LocalDataReplayAdapter: unable to open CSV path: " + path);
        return false;
    }

    std::vector<ReplayTick> ticks;
    ticks.reserve(1 << 20);

    std::string line;
    while (std::getline(ifs, line)) {
        const std::string t = trim(line);
        if (t.empty() || t[0] == '#') {
            continue;
        }

        ReplayTick tick;
        if (!parseCsvLine(t, defaultSymbol, tick)) {
            continue;
        }
        ticks.push_back(std::move(tick));
    }

    if (ticks.empty()) {
        setLastError("LocalDataReplayAdapter: CSV contained no parseable ticks: " + path);
        return false;
    }

    m_ticks = std::move(ticks);
    resetCursor();
    return true;
}

bool LocalDataReplayAdapter::loadBinary(const std::string& path,
                                        const std::string& defaultSymbol)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        setLastError("LocalDataReplayAdapter: unable to open binary path: " + path);
        return false;
    }

    ifs.seekg(0, std::ios::end);
    const std::streamoff bytes = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    if (bytes <= 0 || (bytes % static_cast<std::streamoff>(sizeof(BinaryTickRecord))) != 0) {
        setLastError("LocalDataReplayAdapter: binary size is invalid for tick record layout: " + path);
        return false;
    }

    const std::size_t count = static_cast<std::size_t>(bytes / static_cast<std::streamoff>(sizeof(BinaryTickRecord)));
    std::vector<BinaryTickRecord> raw(count);
    if (!ifs.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(bytes))) {
        setLastError("LocalDataReplayAdapter: failed to read binary payload: " + path);
        return false;
    }

    std::vector<ReplayTick> ticks;
    ticks.reserve(count);
    for (const auto& rec : raw) {
        ReplayTick tick;
        tick.timestampNs = rec.timestampNs;
        tick.bidPrice    = rec.bidPrice;
        tick.bidSize     = rec.bidSize;
        tick.askPrice    = rec.askPrice;
        tick.askSize     = rec.askSize;
        tick.tradePrice  = rec.tradePrice;
        tick.tradeSize   = rec.tradeSize;
        tick.flags       = rec.flags;
        tick.symbol      = rec.symbol[0] ? std::string(rec.symbol) : defaultSymbol;
        if (tick.symbol.empty()) {
            tick.symbol = "UNKNOWN";
        }
        ticks.push_back(std::move(tick));
    }

    if (ticks.empty()) {
        setLastError("LocalDataReplayAdapter: binary contained zero ticks: " + path);
        return false;
    }

    m_ticks = std::move(ticks);
    resetCursor();
    return true;
}

bool LocalDataReplayAdapter::writeBinary(const std::string& path,
                                         const std::vector<ReplayTick>& ticks,
                                         std::string* err)
{
    if (ticks.empty()) {
        if (err) { *err = "LocalDataReplayAdapter::writeBinary requires non-empty ticks"; }
        return false;
    }

    const std::filesystem::path p(path);
    std::filesystem::create_directories(p.parent_path());

    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        if (err) { *err = "LocalDataReplayAdapter::writeBinary failed to open path: " + path; }
        return false;
    }

    BinaryTickRecord rec{};
    for (const auto& tick : ticks) {
        rec.timestampNs = tick.timestampNs;
        rec.bidPrice    = tick.bidPrice;
        rec.bidSize     = tick.bidSize;
        rec.askPrice    = tick.askPrice;
        rec.askSize     = tick.askSize;
        rec.tradePrice  = tick.tradePrice;
        rec.tradeSize   = tick.tradeSize;
        rec.flags       = tick.flags;
        std::memset(rec.symbol, 0, sizeof(rec.symbol));
        if (!tick.symbol.empty()) {
            const std::size_t n = std::min(tick.symbol.size(), sizeof(rec.symbol) - 1);
            std::memcpy(rec.symbol, tick.symbol.data(), n);
        }

        ofs.write(reinterpret_cast<const char*>(&rec), sizeof(rec));
        if (!ofs.good()) {
            if (err) { *err = "LocalDataReplayAdapter::writeBinary failed while writing path: " + path; }
            return false;
        }
    }

    return true;
}

void LocalDataReplayAdapter::setSpeedMode(ReplaySpeedMode mode) noexcept
{
    m_speedMode = mode;
    switch (mode) {
        case ReplaySpeedMode::REALTIME_1X:
            m_speedFactor = 1.0;
            break;
        case ReplaySpeedMode::FAST_10X:
            m_speedFactor = 10.0;
            break;
        case ReplaySpeedMode::MAX_EFFORT:
            m_speedFactor = 0.0;
            break;
    }
    clearPacing();
}

void LocalDataReplayAdapter::setSpeedFactor(double factor) noexcept
{
    if (!std::isfinite(factor) || factor <= 0.0) {
        m_speedMode = ReplaySpeedMode::MAX_EFFORT;
        m_speedFactor = 0.0;
        clearPacing();
        return;
    }

    m_speedFactor = factor;
    if (std::abs(factor - 1.0) < 1e-12) {
        m_speedMode = ReplaySpeedMode::REALTIME_1X;
    } else if (std::abs(factor - 10.0) < 1e-12) {
        m_speedMode = ReplaySpeedMode::FAST_10X;
    } else {
        m_speedMode = ReplaySpeedMode::FAST_10X;
    }
    clearPacing();
}

bool LocalDataReplayAdapter::nextTick(ReplayTick& out)
{
    if (m_cursor >= m_ticks.size()) {
        return false;
    }

    const ReplayTick& cur = m_ticks[m_cursor];
    if (m_cursor > 0 && m_speedMode != ReplaySpeedMode::MAX_EFFORT && m_speedFactor > 0.0) {
        const uint64_t replayDelayNs = computeReplayDelayNs(m_lastReplayTick, cur);
        if (replayDelayNs > 0) {
            const auto now = std::chrono::steady_clock::now();
            if (!m_hasLastWallClock) {
                m_lastWallClock = now;
                m_hasLastWallClock = true;
            }
            const auto due = m_lastWallClock + std::chrono::nanoseconds(replayDelayNs);
            if (due > now) {
                std::this_thread::sleep_for(due - now);
            }
            m_lastWallClock = due;
        }
    } else if (!m_hasLastWallClock) {
        m_lastWallClock = std::chrono::steady_clock::now();
        m_hasLastWallClock = true;
    }

    out = cur;
    m_lastReplayTick = cur;
    ++m_cursor;
    return true;
}

bool LocalDataReplayAdapter::seek(std::size_t idx) noexcept
{
    if (idx >= m_ticks.size()) {
        return false;
    }
    m_cursor = idx;
    clearPacing();
    return true;
}

void LocalDataReplayAdapter::resetCursor() noexcept
{
    m_cursor = 0;
    clearPacing();
}

std::size_t LocalDataReplayAdapter::size() const noexcept
{
    return m_ticks.size();
}

std::size_t LocalDataReplayAdapter::cursor() const noexcept
{
    return m_cursor;
}

bool LocalDataReplayAdapter::empty() const noexcept
{
    return m_ticks.empty();
}

std::string LocalDataReplayAdapter::lastError() const
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    return m_lastError;
}

bool LocalDataReplayAdapter::parseCsvLine(const std::string& line,
                                          const std::string& defaultSymbol,
                                          ReplayTick& out) const
{
    const std::vector<std::string> cols = splitCsv(line);
    if (cols.size() < 5) {
        return false;
    }

    uint64_t timestamp = 0;
    if (!parseU64(cols[0], timestamp)) {
        return false;
    }

    double bidPrice = 0.0;
    double bidSize = 0.0;
    double askPrice = 0.0;
    double askSize = 0.0;
    double tradePrice = 0.0;
    double tradeSize = 0.0;

    if (!parseDouble(cols[1], bidPrice)
        || !parseDouble(cols[2], bidSize)
        || !parseDouble(cols[3], askPrice)
        || !parseDouble(cols[4], askSize)) {
        return false;
    }

    if (cols.size() > 5) { (void)parseDouble(cols[5], tradePrice); }
    if (cols.size() > 6) { (void)parseDouble(cols[6], tradeSize); }

    std::string symbol = defaultSymbol;
    if (cols.size() > 7) {
        symbol = trim(cols[7]);
    }
    if (symbol.empty()) {
        symbol = "UNKNOWN";
    }

    if (tradePrice <= 0.0) {
        tradePrice = 0.5 * (bidPrice + askPrice);
    }

    out.timestampNs = timestamp;
    out.symbol      = symbol;
    out.bidPrice    = bidPrice;
    out.bidSize     = bidSize;
    out.askPrice    = askPrice;
    out.askSize     = askSize;
    out.tradePrice  = tradePrice;
    out.tradeSize   = tradeSize;
    out.flags       = (tradeSize > 0.0) ? 1u : 0u;
    return true;
}

bool LocalDataReplayAdapter::hasBinaryExtension(const std::string& path) noexcept
{
    const std::filesystem::path p(path);
    const auto ext = p.extension().string();
    return ext == ".bin" || ext == ".l2b";
}

void LocalDataReplayAdapter::clearPacing() noexcept
{
    m_hasLastWallClock = false;
    m_lastReplayTick = ReplayTick{};
}

uint64_t LocalDataReplayAdapter::computeReplayDelayNs(const ReplayTick& prev,
                                                      const ReplayTick& cur) const noexcept
{
    if (cur.timestampNs <= prev.timestampNs) {
        return 0;
    }
    const uint64_t rawDelta = cur.timestampNs - prev.timestampNs;
    if (m_speedFactor <= 0.0) {
        return 0;
    }
    const double scaled = static_cast<double>(rawDelta) / m_speedFactor;
    if (scaled <= 0.0) {
        return 0;
    }
    return static_cast<uint64_t>(scaled);
}

void LocalDataReplayAdapter::setLastError(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(m_errorMutex);
    m_lastError = msg;
}
