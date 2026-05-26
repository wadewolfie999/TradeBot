#include "StateSerializer.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <limits>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

// ── Writer helpers ────────────────────────────────────────────────────────────

std::string StateSerializer::writeDouble(double v)
{
    std::ostringstream oss;
    oss << std::setprecision(std::numeric_limits<double>::max_digits10) << v;
    return oss.str();
}

std::string StateSerializer::writeUint64(uint64_t v)
{
    return std::to_string(v);
}

std::string StateSerializer::writeBool(bool v)
{
    return v ? "true" : "false";
}

// ── saveSnapshot ─────────────────────────────────────────────────────────────

bool StateSerializer::saveSnapshot(const PortfolioManager& portfolio,
                                   const RiskEngine&       riskEngine,
                                   uint64_t                checkpointTs,
                                   const std::string&      filepath) const
{
    m_lastError.clear();

    // Ensure parent directory exists.
    try {
        const auto parent = std::filesystem::path(filepath).parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent);
        }
    } catch (const std::exception& ex) {
        m_lastError = "create_directories failed: ";
        m_lastError += ex.what();
        return false;
    }

    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
        m_lastError = "Cannot open file for writing: " + filepath;
        return false;
    }

    // Shorthand lambdas.
    auto wd = [](double v)   { return StateSerializer::writeDouble(v); };
    auto wu = [](uint64_t v) { return StateSerializer::writeUint64(v); };
    auto wb = [](bool v)     { return StateSerializer::writeBool(v); };

    ofs << "{\n";
    ofs << "  \"version\": 9,\n";
    ofs << "  \"checkpointTimestamp\": " << wu(checkpointTs) << ",\n";

    // ── Portfolio ────────────────────────────────────────────────────────
    ofs << "  \"portfolio\": {\n";
    ofs << "    \"cash\": "           << wd(portfolio.getCashBalance())  << ",\n";
    ofs << "    \"totalFeesPaid\": "  << wd(portfolio.getTotalFeesPaid()) << ",\n";
    ofs << "    \"maxEquity\": "      << wd(portfolio.getTotalEquity())  << ",\n";
    ofs << "    \"maxDrawdown\": "    << wd(portfolio.getMaxDrawdown())  << ",\n";
    ofs << "    \"tradeCount\": "     << portfolio.getTradeCount()       << ",\n";
    ofs << "    \"roundTripCount\": " << portfolio.getRoundTripCount()   << ",\n";

    // ── Open positions ────────────────────────────────────────────────
    // We expose positions via the helper methods; serialise symbol + quantity +
    // entryPrice by iterating the pending-order-independent fields we can query.
    // NOTE: PortfolioManager does not expose an iterator over positions; we work
    // around this by reading the serialisation-accessor that we added.
    // We use a friend-free approach: the positions map is private, so we expose
    // a dedicated serialization accessor: getPositionSymbols() if needed.
    // For now we iterate over a snapshot obtained through the trade log + open
    // position queries.  Because PortfolioManager::m_positions is private we
    // need to expose enough data.  Rather than pollute the public interface with
    // an iterator, we add a minimal accessor here via the existing public API:
    //   - hasPosition(symbol) + getPositionQuantity() + getEntryPrice()
    // We don't have the symbol list directly; but we can iterate the trade log
    // to reconstruct which symbols have been traded, then filter open ones.
    // A cleaner solution: expose a const accessor for the position snapshot.
    // For Phase 9 we add a lightweight helper to PortfolioManager.
    //
    // Implementation: collect open symbols from the pending-order queue and
    // from previously seen symbols via a helper that we define inline here.
    // Since we cannot enumerate positions without a symbol-list accessor, we
    // add getOpenSymbols() at the bottom of this file using the tradeLog +
    // open check approach, but the *correct* fix is the friend declaration
    // or a public accessor.  We expose via a private-section accessor added
    // in a later refactor OR we just expose an opaque state blob.
    //
    // For correctness and clean encapsulation we use a dedicated
    // PortfolioManager::getPositionSnapshot() accessor defined below.
    // We document this as an internal serialisation contract.
    //
    // Implementation: temporarily extend PortfolioManager with a snapshot
    // method.  Since we cannot change the header between patches we use a
    // pragmatic approach: we serialise positions through a parallel method
    // added at the end of PortfolioManager.cpp later in this patch set.
    //
    // For now: we write positions as an empty array and note that the
    // full implementation patches PortfolioManager to add getPositionSnapshot().
    // ─────────────────────────────────────────────────────────────────────
    ofs << "    \"positions\": [\n";
    {
        // Collect open symbols by scanning the trade log for buys without
        // matching sells, and checking hasPosition().  This is O(N) in trade
        // count and correct because every open position must have had an entry
        // trade.  We accumulate unique symbols.
        const auto& log = portfolio.getTradeLog();
        std::vector<std::string> seenSymbols;
        for (const auto& tr : log) {
            if (std::find(seenSymbols.begin(), seenSymbols.end(), tr.symbol)
                == seenSymbols.end()) {
                seenSymbols.push_back(tr.symbol);
            }
        }
        // Also check pending orders for symbols that may have never had a
        // closed trade.
        for (const auto& ord : portfolio.getPendingOrders()) {
            if (std::find(seenSymbols.begin(), seenSymbols.end(), ord.symbol)
                == seenSymbols.end()) {
                seenSymbols.push_back(ord.symbol);
            }
        }

        bool first = true;
        for (const auto& sym : seenSymbols) {
            if (!portfolio.hasPosition(sym)) { continue; }
            if (!first) { ofs << ",\n"; }
            first = false;
            ofs << "      {\n";
            ofs << "        \"symbol\": \""     << sym << "\",\n";
            ofs << "        \"quantity\": "     << wd(portfolio.getPositionQuantity(sym)) << ",\n";
            ofs << "        \"entryPrice\": "   << wd(portfolio.getEntryPrice(sym)) << ",\n";
            ofs << "        \"entryTimestamp\": 0,\n";  // best-effort (not directly exposed)
            ofs << "        \"entryFee\": 0.0\n";       // best-effort
            ofs << "      }";
        }
        if (!first) { ofs << "\n"; }
    }
    ofs << "    ],\n";

    // ── Pending orders ────────────────────────────────────────────────
    ofs << "    \"pendingOrders\": [\n";
    {
        bool first = true;
        for (const auto& ord : portfolio.getPendingOrders()) {
            if (!first) { ofs << ",\n"; }
            first = false;
            ofs << "      {\n";
            ofs << "        \"orderId\": "          << wu(ord.orderId)   << ",\n";
            ofs << "        \"symbol\": \""         << ord.symbol        << "\",\n";
            ofs << "        \"orderType\": "        << static_cast<int>(ord.orderType) << ",\n";
            ofs << "        \"isBuy\": "            << wb(ord.isBuy)     << ",\n";
            ofs << "        \"limitPrice\": "       << wd(ord.limitPrice)       << ",\n";
            ofs << "        \"trailOffset\": "      << wd(ord.trailOffset)      << ",\n";
            ofs << "        \"trailBest\": "        << wd(ord.trailBest)        << ",\n";
            ofs << "        \"quantity\": "         << wd(ord.quantity)         << ",\n";
            ofs << "        \"capitalToCommit\": "  << wd(ord.capitalToCommit)  << ",\n";
            ofs << "        \"placedTimestamp\": "  << wu(ord.placedTimestamp)  << "\n";
            ofs << "      }";
        }
        if (!first) { ofs << "\n"; }
    }
    ofs << "    ]\n";
    ofs << "  },\n";

    // ── RiskEngine ───────────────────────────────────────────────────────
    ofs << "  \"riskEngine\": {\n";
    ofs << "    \"totalDrawdown\": "  << wd(0.0) << ",\n"; // re-computed on resume
    ofs << "    \"dailyDrawdown\": "  << wd(0.0) << ",\n";

    // Legacy single-asset rolling return window (Phase 8 backward compat).
    // We expose these via getReturnStdDev (indirect); no direct buffer accessor,
    // so we store a comment placeholder and rely on per-asset states for resume.
    ofs << "    \"returnStdDev\": "   << wd(riskEngine.getReturnStdDev()) << ",\n";
    ofs << "    \"var95\": "          << wd(riskEngine.getVaR95())        << ",\n";

    // Per-asset return state.
    ofs << "    \"assetStates\": [\n";
    {
        const auto& states = riskEngine.getAssetReturnStates();
        bool first = true;
        for (const auto& [sym, st] : states) {
            if (!first) { ofs << ",\n"; }
            first = false;
            ofs << "      {\n";
            ofs << "        \"symbol\": \""         << sym                  << "\",\n";
            ofs << "        \"prevPrice\": "         << wd(st.prevPrice)     << ",\n";
            ofs << "        \"prevPriceValid\": "    << wb(st.prevPriceValid)<< ",\n";
            ofs << "        \"positionValue\": "     << wd(st.positionValue) << ",\n";
            ofs << "        \"returnWindow\": [";
            bool fw = true;
            for (double r : st.returnWindow) {
                if (!fw) { ofs << ","; }
                fw = false;
                ofs << wd(r);
            }
            ofs << "]\n";
            ofs << "      }";
        }
        if (!first) { ofs << "\n"; }
    }
    ofs << "    ],\n";

    // Covariance matrix (row-major flat array).
    ofs << "    \"assetOrder\": [";
    {
        const auto& order = riskEngine.getAssetOrder();
        bool first = true;
        for (const auto& sym : order) {
            if (!first) { ofs << ","; }
            first = false;
            ofs << "\"" << sym << "\"";
        }
    }
    ofs << "],\n";

    ofs << "    \"covMatrix\": [";
    {
        const auto& cov = riskEngine.getCovarianceMatrix();
        bool first = true;
        for (double v : cov) {
            if (!first) { ofs << ","; }
            first = false;
            ofs << wd(v);
        }
    }
    ofs << "]\n";

    ofs << "  }\n";
    ofs << "}\n";

    if (!ofs.good()) {
        m_lastError = "Write error on file: " + filepath;
        return false;
    }

    return true;
}

// ── Parser helpers ────────────────────────────────────────────────────────────

void StateSerializer::skipWs(ParseCtx& ctx)
{
    while (ctx.p < ctx.end &&
           (*ctx.p == ' ' || *ctx.p == '\t' || *ctx.p == '\n' || *ctx.p == '\r')) {
        ++ctx.p;
    }
}

bool StateSerializer::expect(ParseCtx& ctx, char c)
{
    skipWs(ctx);
    if (ctx.p >= ctx.end || *ctx.p != c) {
        ctx.error = std::string("Expected '") + c + "' at offset "
                  + std::to_string(ctx.p - (ctx.end - std::strlen(ctx.end)));
        return false;
    }
    ++ctx.p;
    return true;
}

bool StateSerializer::parseString(ParseCtx& ctx, std::string& out)
{
    skipWs(ctx);
    if (ctx.p >= ctx.end || *ctx.p != '"') {
        ctx.error = "Expected '\"'";
        return false;
    }
    ++ctx.p;
    out.clear();
    while (ctx.p < ctx.end && *ctx.p != '"') {
        if (*ctx.p == '\\') {
            ++ctx.p;
            if (ctx.p < ctx.end) { out += *ctx.p; ++ctx.p; }
        } else {
            out += *ctx.p++;
        }
    }
    if (ctx.p >= ctx.end) { ctx.error = "Unterminated string"; return false; }
    ++ctx.p; // consume closing '"'
    return true;
}

bool StateSerializer::parseDouble(ParseCtx& ctx, double& out)
{
    skipWs(ctx);
    // Handle "null" as 0.0 (defensive).
    if (ctx.p + 4 <= ctx.end && std::strncmp(ctx.p, "null", 4) == 0) {
        ctx.p += 4;
        out = 0.0;
        return true;
    }
    char* endp = nullptr;
    errno = 0;
    out = std::strtod(ctx.p, &endp);
    if (endp == ctx.p) { ctx.error = "Expected double"; return false; }
    ctx.p = endp;
    return true;
}

bool StateSerializer::parseUint64(ParseCtx& ctx, uint64_t& out)
{
    skipWs(ctx);
    char* endp = nullptr;
    errno = 0;
    unsigned long long v = std::strtoull(ctx.p, &endp, 10);
    if (endp == ctx.p) { ctx.error = "Expected uint64"; return false; }
    ctx.p = endp;
    out = static_cast<uint64_t>(v);
    return true;
}

bool StateSerializer::parseInt(ParseCtx& ctx, int& out)
{
    skipWs(ctx);
    char* endp = nullptr;
    long v = std::strtol(ctx.p, &endp, 10);
    if (endp == ctx.p) { ctx.error = "Expected int"; return false; }
    ctx.p = endp;
    out = static_cast<int>(v);
    return true;
}

bool StateSerializer::parseBool(ParseCtx& ctx, bool& out)
{
    skipWs(ctx);
    if (ctx.p + 4 <= ctx.end && std::strncmp(ctx.p, "true", 4) == 0) {
        ctx.p += 4; out = true;  return true;
    }
    if (ctx.p + 5 <= ctx.end && std::strncmp(ctx.p, "false", 5) == 0) {
        ctx.p += 5; out = false; return true;
    }
    ctx.error = "Expected bool";
    return false;
}

bool StateSerializer::parseDoubleArray(ParseCtx& ctx, std::vector<double>& out)
{
    skipWs(ctx);
    if (!expect(ctx, '[')) { return false; }
    out.clear();
    skipWs(ctx);
    if (ctx.p < ctx.end && *ctx.p == ']') { ++ctx.p; return true; }
    while (ctx.p < ctx.end) {
        double v = 0.0;
        if (!parseDouble(ctx, v)) { return false; }
        out.push_back(v);
        skipWs(ctx);
        if (ctx.p < ctx.end && *ctx.p == ',') { ++ctx.p; continue; }
        break;
    }
    return expect(ctx, ']');
}

bool StateSerializer::parseStringArray(ParseCtx& ctx, std::vector<std::string>& out)
{
    skipWs(ctx);
    if (!expect(ctx, '[')) { return false; }
    out.clear();
    skipWs(ctx);
    if (ctx.p < ctx.end && *ctx.p == ']') { ++ctx.p; return true; }
    while (ctx.p < ctx.end) {
        std::string s;
        if (!parseString(ctx, s)) { return false; }
        out.push_back(s);
        skipWs(ctx);
        if (ctx.p < ctx.end && *ctx.p == ',') { ++ctx.p; continue; }
        break;
    }
    return expect(ctx, ']');
}

// ── loadSnapshot ─────────────────────────────────────────────────────────────
// We use a key-scanning approach: locate JSON keys by string search then parse
// the associated value.  This is robust for our fixed, shallow schema.

bool StateSerializer::scanKey(ParseCtx& ctx, const std::string& key)
{
    // Reset p to scan for the quoted key from current position.
    // We simply search forward; we don't recurse into sub-objects.
    const std::string needle = "\"" + key + "\"";
    const char* found = std::search(ctx.p, ctx.end,
                                    needle.c_str(), needle.c_str() + needle.size());
    if (found == ctx.end) {
        ctx.error = "Key not found: " + key;
        return false;
    }
    ctx.p = found + needle.size();
    StateSerializer::skipWs(ctx);
    if (ctx.p >= ctx.end || *ctx.p != ':') {
        ctx.error = "Expected ':' after key " + key;
        return false;
    }
    ++ctx.p;
    return true;
}

bool StateSerializer::loadSnapshot(PortfolioManager& portfolio,
                                   RiskEngine&       riskEngine,
                                   uint64_t&         checkpointTs,
                                   const std::string& filepath) const
{
    m_lastError.clear();

    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        m_lastError = "Cannot open snapshot file: " + filepath;
        return false;
    }
    std::ostringstream buf;
    buf << ifs.rdbuf();
    const std::string raw = buf.str();

    ParseCtx ctx;
    ctx.p   = raw.c_str();
    ctx.end = raw.c_str() + raw.size();

    // ── Checkpoint timestamp ─────────────────────────────────────────────
    {
        ParseCtx tmp = ctx;
        if (!scanKey(tmp, "checkpointTimestamp")) {
            m_lastError = tmp.error;
            return false;
        }
        if (!parseUint64(tmp, checkpointTs)) {
            m_lastError = tmp.error;
            return false;
        }
    }

    // ── Portfolio: cash ──────────────────────────────────────────────────
    // We reconstruct the portfolio state by calling the existing public API.
    // Because the constructor fixes STARTING_CASH and private state, we
    // restore by directly reading what we serialised and then setting internal
    // state via the available mutator path.
    //
    // Cash is not directly settable; the most portable approach is to use
    // the portfolio's internal accumulator by calling openLong / closePosition
    // sequences OR to add a dedicated restore accessor.  We added
    // restorePendingOrders(); for full state we need a bulk-restore method.
    //
    // Solution: We expose a restorePortfolioState() method in PortfolioManager
    // (defined below in PortfolioManager.cpp).  This keeps mutation encapsulated.

    double cash = 0.0;
    {
        ParseCtx tmp = ctx;
        if (!scanKey(tmp, "cash")) { m_lastError = tmp.error; return false; }
        if (!parseDouble(tmp, cash)) { m_lastError = tmp.error; return false; }
    }

    // ── Portfolio: positions ─────────────────────────────────────────────
    struct PosSnap {
        std::string symbol;
        double quantity{0.0};
        double entryPrice{0.0};
        uint64_t entryTimestamp{0};
        double   entryFee{0.0};
    };
    std::vector<PosSnap> positions;
    {
        ParseCtx tmp = ctx;
        if (!scanKey(tmp, "positions")) { m_lastError = tmp.error; return false; }
        skipWs(tmp);
        if (!expect(tmp, '[')) { m_lastError = tmp.error; return false; }
        skipWs(tmp);
        while (tmp.p < tmp.end && *tmp.p != ']') {
            skipWs(tmp);
            if (*tmp.p != '{') { break; }
            ++tmp.p;
            PosSnap ps;
            // Parse fields in any order.
            for (int fi = 0; fi < 5; ++fi) {
                skipWs(tmp);
                if (tmp.p < tmp.end && *tmp.p == '}') { break; }
                std::string key;
                if (!parseString(tmp, key)) break;
                skipWs(tmp);
                if (tmp.p < tmp.end && *tmp.p == ':') { ++tmp.p; }
                if      (key == "symbol")         { parseString(tmp, ps.symbol); }
                else if (key == "quantity")        { parseDouble(tmp, ps.quantity); }
                else if (key == "entryPrice")      { parseDouble(tmp, ps.entryPrice); }
                else if (key == "entryTimestamp")  { parseUint64(tmp, ps.entryTimestamp); }
                else if (key == "entryFee")        { parseDouble(tmp, ps.entryFee); }
                skipWs(tmp);
                if (tmp.p < tmp.end && *tmp.p == ',') { ++tmp.p; }
            }
            skipWs(tmp);
            if (tmp.p < tmp.end && *tmp.p == '}') { ++tmp.p; }
            positions.push_back(ps);
            skipWs(tmp);
            if (tmp.p < tmp.end && *tmp.p == ',') { ++tmp.p; }
            skipWs(tmp);
        }
    }

    // ── Portfolio: pending orders ─────────────────────────────────────────
    std::list<OrderRecord> pendingOrders;
    {
        ParseCtx tmp = ctx;
        if (!scanKey(tmp, "pendingOrders")) { m_lastError = tmp.error; return false; }
        skipWs(tmp);
        if (!expect(tmp, '[')) { m_lastError = tmp.error; return false; }
        skipWs(tmp);
        while (tmp.p < tmp.end && *tmp.p != ']') {
            skipWs(tmp);
            if (*tmp.p != '{') { break; }
            ++tmp.p;
            OrderRecord ord;
            for (int fi = 0; fi < 10; ++fi) {
                skipWs(tmp);
                if (tmp.p < tmp.end && *tmp.p == '}') { break; }
                std::string key;
                if (!parseString(tmp, key)) break;
                skipWs(tmp);
                if (tmp.p < tmp.end && *tmp.p == ':') { ++tmp.p; }
                if (key == "orderId")          { parseUint64(tmp, ord.orderId); }
                else if (key == "symbol")      { parseString(tmp, ord.symbol); }
                else if (key == "orderType")   {
                    int ot = 0; parseInt(tmp, ot);
                    ord.orderType = static_cast<OrderType>(ot);
                }
                else if (key == "isBuy")           { parseBool(tmp, ord.isBuy); }
                else if (key == "limitPrice")      { parseDouble(tmp, ord.limitPrice); }
                else if (key == "trailOffset")     { parseDouble(tmp, ord.trailOffset); }
                else if (key == "trailBest")       { parseDouble(tmp, ord.trailBest); }
                else if (key == "quantity")        { parseDouble(tmp, ord.quantity); }
                else if (key == "capitalToCommit") { parseDouble(tmp, ord.capitalToCommit); }
                else if (key == "placedTimestamp") { parseUint64(tmp, ord.placedTimestamp); }
                skipWs(tmp);
                if (tmp.p < tmp.end && *tmp.p == ',') { ++tmp.p; }
            }
            skipWs(tmp);
            if (tmp.p < tmp.end && *tmp.p == '}') { ++tmp.p; }
            pendingOrders.push_back(ord);
            skipWs(tmp);
            if (tmp.p < tmp.end && *tmp.p == ',') { ++tmp.p; }
            skipWs(tmp);
        }
    }

    double totalFeesPaid = 0.0;
    double maxEquity     = 0.0;
    double maxDrawdown   = 0.0;
    int    tradeCount    = 0;
    int    roundTrips    = 0;
    {
        ParseCtx tmp = ctx;
        if (scanKey(tmp, "totalFeesPaid")) parseDouble(tmp, totalFeesPaid);
    }
    {
        ParseCtx tmp = ctx;
        if (scanKey(tmp, "maxEquity"))    parseDouble(tmp, maxEquity);
    }
    {
        ParseCtx tmp = ctx;
        if (scanKey(tmp, "maxDrawdown"))  parseDouble(tmp, maxDrawdown);
    }
    {
        ParseCtx tmp = ctx;
        if (scanKey(tmp, "tradeCount"))   parseInt(tmp, tradeCount);
    }
    {
        ParseCtx tmp = ctx;
        if (scanKey(tmp, "roundTripCount")) parseInt(tmp, roundTrips);
    }

    // ── RiskEngine: asset states ─────────────────────────────────────────
    std::unordered_map<std::string, RiskEngine::AssetReturnState> assetStates;
    {
        ParseCtx tmp = ctx;
        if (!scanKey(tmp, "assetStates")) { m_lastError = tmp.error; return false; }
        skipWs(tmp);
        if (!expect(tmp, '[')) { m_lastError = tmp.error; return false; }
        skipWs(tmp);
        while (tmp.p < tmp.end && *tmp.p != ']') {
            skipWs(tmp);
            if (*tmp.p != '{') break;
            ++tmp.p;
            RiskEngine::AssetReturnState st;
            for (int fi = 0; fi < 5; ++fi) {
                skipWs(tmp);
                if (tmp.p < tmp.end && *tmp.p == '}') break;
                std::string key;
                if (!parseString(tmp, key)) break;
                skipWs(tmp);
                if (tmp.p < tmp.end && *tmp.p == ':') { ++tmp.p; }
                if      (key == "symbol")       { parseString(tmp, st.symbol); }
                else if (key == "prevPrice")     { parseDouble(tmp, st.prevPrice); }
                else if (key == "prevPriceValid"){ parseBool(tmp, st.prevPriceValid); }
                else if (key == "positionValue") { parseDouble(tmp, st.positionValue); }
                else if (key == "returnWindow")  {
                    std::vector<double> rv;
                    parseDoubleArray(tmp, rv);
                    st.returnWindow.assign(rv.begin(), rv.end());
                }
                skipWs(tmp);
                if (tmp.p < tmp.end && *tmp.p == ',') { ++tmp.p; }
            }
            skipWs(tmp);
            if (tmp.p < tmp.end && *tmp.p == '}') { ++tmp.p; }
            if (!st.symbol.empty()) { assetStates[st.symbol] = st; }
            skipWs(tmp);
            if (tmp.p < tmp.end && *tmp.p == ',') { ++tmp.p; }
            skipWs(tmp);
        }
    }

    std::vector<std::string> assetOrder;
    std::vector<double>      covMatrix;
    {
        ParseCtx tmp = ctx;
        if (scanKey(tmp, "assetOrder")) parseStringArray(tmp, assetOrder);
    }
    {
        ParseCtx tmp = ctx;
        if (scanKey(tmp, "covMatrix"))  parseDoubleArray(tmp, covMatrix);
    }

    // ── Commit restored state ────────────────────────────────────────────
    portfolio.restoreState(cash, positions.empty() ? std::vector<std::tuple<std::string,double,double,uint64_t,double>>{} :
        [&positions]() {
            std::vector<std::tuple<std::string,double,double,uint64_t,double>> v;
            v.reserve(positions.size());
            for (const auto& ps : positions)
                v.emplace_back(ps.symbol, ps.quantity, ps.entryPrice, ps.entryTimestamp, ps.entryFee);
            return v;
        }(),
        totalFeesPaid, maxEquity, maxDrawdown, tradeCount, roundTrips);

    portfolio.restorePendingOrders(pendingOrders);

    riskEngine.restoreAssetReturnState(assetStates);
    riskEngine.restoreCovarianceMatrix(assetOrder, covMatrix);

    return true;
}

const std::string& StateSerializer::lastError() const noexcept
{
    return m_lastError;
}

// ── Phase 11: saveSnapshot (with RegimeDetector + PortfolioAllocator Phi) ─────

bool StateSerializer::saveSnapshot(const PortfolioManager&   portfolio,
                                   const RiskEngine&         riskEngine,
                                   const RegimeDetector&     regimeDetector,
                                   const PortfolioAllocator& allocator,
                                   uint64_t                  checkpointTs,
                                   const std::string&        filepath) const
{
    m_lastError.clear();

    try {
        const auto parent = std::filesystem::path(filepath).parent_path();
        if (!parent.empty()) { std::filesystem::create_directories(parent); }
    } catch (const std::exception& ex) {
        m_lastError = std::string("create_directories failed: ") + ex.what();
        return false;
    }

    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
        m_lastError = "Cannot open file for writing: " + filepath;
        return false;
    }

    auto wd = [](double v)   { return StateSerializer::writeDouble(v); };
    auto wu = [](uint64_t v) { return StateSerializer::writeUint64(v); };
    auto wb = [](bool v)     { return StateSerializer::writeBool(v); };

    // ── Re-use the Phase 9 save logic for portfolio + riskEngine ─────────
    // To avoid code duplication we close the temp file and delegate; but since
    // we cannot call the other overload (it would open its own file), we inline
    // the shared portfolio/riskEngine write here.  We mark version 11.

    ofs << "{\n";
    ofs << "  \"version\": 11,\n";
    ofs << "  \"checkpointTimestamp\": " << wu(checkpointTs) << ",\n";

    // ── Portfolio (identical to Phase 9 block) ────────────────────────────
    ofs << "  \"portfolio\": {\n";
    ofs << "    \"cash\": "           << wd(portfolio.getCashBalance())  << ",\n";
    ofs << "    \"totalFeesPaid\": "  << wd(portfolio.getTotalFeesPaid()) << ",\n";
    ofs << "    \"maxEquity\": "      << wd(portfolio.getTotalEquity())  << ",\n";
    ofs << "    \"maxDrawdown\": "    << wd(portfolio.getMaxDrawdown())  << ",\n";
    ofs << "    \"tradeCount\": "     << portfolio.getTradeCount()       << ",\n";
    ofs << "    \"roundTripCount\": " << portfolio.getRoundTripCount()   << ",\n";

    ofs << "    \"positions\": [\n";
    {
        const auto& log = portfolio.getTradeLog();
        std::vector<std::string> seenSyms;
        for (const auto& t : log) {
            if (std::find(seenSyms.begin(), seenSyms.end(), t.symbol) == seenSyms.end())
                seenSyms.push_back(t.symbol);
        }
        bool firstPos = true;
        for (const auto& sym : seenSyms) {
            if (!portfolio.hasPosition(sym)) { continue; }
            if (!firstPos) { ofs << ",\n"; }
            firstPos = false;
            ofs << "      {\n";
            ofs << "        \"symbol\": \""     << sym                                     << "\",\n";
            ofs << "        \"quantity\": "     << wd(portfolio.getPositionQuantity(sym))  << ",\n";
            ofs << "        \"entryPrice\": "   << wd(portfolio.getEntryPrice(sym))        << ",\n";
            ofs << "        \"entryTimestamp\": 0,\n";
            ofs << "        \"entryFee\": 0.0\n";
            ofs << "      }";
        }
        if (!firstPos) { ofs << "\n"; }
    }
    ofs << "    ],\n";

    ofs << "    \"pendingOrders\": [\n";
    {
        const auto& orders = portfolio.getPendingOrders();
        bool firstOrd = true;
        for (const auto& ord : orders) {
            if (!firstOrd) { ofs << ",\n"; }
            firstOrd = false;
            ofs << "      {\n";
            ofs << "        \"orderId\": "          << wu(ord.orderId)                     << ",\n";
            ofs << "        \"symbol\": \""         << ord.symbol                          << "\",\n";
            ofs << "        \"orderType\": "        << static_cast<int>(ord.orderType)     << ",\n";
            ofs << "        \"isBuy\": "            << wb(ord.isBuy)                       << ",\n";
            ofs << "        \"limitPrice\": "       << wd(ord.limitPrice)                  << ",\n";
            ofs << "        \"trailOffset\": "      << wd(ord.trailOffset)                 << ",\n";
            ofs << "        \"trailBest\": "        << wd(ord.trailBest)                   << ",\n";
            ofs << "        \"quantity\": "         << wd(ord.quantity)                    << ",\n";
            ofs << "        \"capitalToCommit\": "  << wd(ord.capitalToCommit)             << ",\n";
            ofs << "        \"placedTimestamp\": "  << wu(ord.placedTimestamp)             << "\n";
            ofs << "      }";
        }
        if (!firstOrd) { ofs << "\n"; }
    }
    ofs << "    ]\n";
    ofs << "  },\n";

    // ── RiskEngine ────────────────────────────────────────────────────────
    ofs << "  \"riskEngine\": {\n";
    ofs << "    \"totalDrawdown\": "  << wd(0.0) << ",\n";
    ofs << "    \"dailyDrawdown\": "  << wd(0.0) << ",\n";
    ofs << "    \"returnStdDev\": "   << wd(riskEngine.getReturnStdDev()) << ",\n";
    ofs << "    \"var95\": "          << wd(riskEngine.getVaR95())        << ",\n";
    ofs << "    \"prevEquity\": "     << wd(0.0) << ",\n";
    ofs << "    \"prevEquityValid\": false,\n";
    ofs << "    \"returnWindow\": [],\n";

    ofs << "    \"assetStates\": [\n";
    {
        const auto& states = riskEngine.getAssetReturnStates();
        bool first = true;
        for (const auto& [sym, st] : states) {
            if (!first) { ofs << ",\n"; }
            first = false;
            ofs << "      {\n";
            ofs << "        \"symbol\": \""         << sym                << "\",\n";
            ofs << "        \"prevPrice\": "         << wd(st.prevPrice)   << ",\n";
            ofs << "        \"prevPriceValid\": "    << wb(st.prevPriceValid) << ",\n";
            ofs << "        \"positionValue\": "     << wd(st.positionValue) << ",\n";
            ofs << "        \"returnWindow\": [";
            bool fw = true;
            for (double rv : st.returnWindow) {
                if (!fw) { ofs << ","; }
                fw = false;
                ofs << wd(rv);
            }
            ofs << "]\n";
            ofs << "      }";
        }
        if (!first) { ofs << "\n"; }
    }
    ofs << "    ],\n";

    ofs << "    \"assetOrder\": [";
    {
        const auto& order = riskEngine.getAssetOrder();
        bool first = true;
        for (const auto& sym : order) {
            if (!first) { ofs << ","; }
            first = false;
            ofs << "\"" << sym << "\"";
        }
    }
    ofs << "],\n";

    ofs << "    \"covMatrix\": [";
    {
        const auto& cov = riskEngine.getCovarianceMatrix();
        bool first = true;
        for (double v : cov) {
            if (!first) { ofs << ","; }
            first = false;
            ofs << wd(v);
        }
    }
    ofs << "]\n";
    ofs << "  },\n";

    // ── RegimeDetector state ──────────────────────────────────────────────
    const auto& rs = regimeDetector.getState();
    ofs << "  \"regimeDetector\": {\n";
    ofs << "    \"smoothTR\": "        << wd(rs.smoothTR)        << ",\n";
    ofs << "    \"smoothDMPlus\": "    << wd(rs.smoothDMPlus)    << ",\n";
    ofs << "    \"smoothDMMinus\": "   << wd(rs.smoothDMMinus)   << ",\n";
    ofs << "    \"adx\": "             << wd(rs.adx)             << ",\n";
    ofs << "    \"adxObservations\": " << rs.adxObservations      << ",\n";
    ofs << "    \"prevClose\": "       << wd(rs.prevClose)       << ",\n";
    ofs << "    \"prevCloseValid\": "  << wb(rs.prevCloseValid)  << ",\n";
    ofs << "    \"variance\": "        << wd(rs.variance)        << ",\n";
    ofs << "    \"prevHigh\": "        << wd(rs.prevHigh)        << ",\n";
    ofs << "    \"prevLow\": "         << wd(rs.prevLow)         << ",\n";
    ofs << "    \"prevHLValid\": "     << wb(rs.prevHLValid)     << ",\n";
    ofs << "    \"regime\": "          << static_cast<int>(rs.regime) << ",\n";
    ofs << "    \"returnWindow\": [";
    {
        bool first = true;
        for (double v : rs.returnWindow) {
            if (!first) { ofs << ","; }
            first = false;
            ofs << wd(v);
        }
    }
    ofs << "]\n";
    ofs << "  },\n";

    // ── PortfolioAllocator Phi states ─────────────────────────────────────
    ofs << "  \"allocatorPhi\": [\n";
    {
        const auto& phiMap = allocator.getPhiStates();
        bool firstPhi = true;
        for (const auto& [sid, ps] : phiMap) {
            if (!firstPhi) { ofs << ",\n"; }
            firstPhi = false;
            ofs << "    {\n";
            ofs << "      \"strategyId\": \"" << sid  << "\",\n";
            ofs << "      \"phi\": "          << wd(ps.phi) << ",\n";
            ofs << "      \"pnlWindow\": [";
            bool fw = true;
            for (double v : ps.pnlWindow) {
                if (!fw) { ofs << ","; }
                fw = false;
                ofs << wd(v);
            }
            ofs << "]\n";
            ofs << "    }";
        }
        if (!firstPhi) { ofs << "\n"; }
    }
    ofs << "  ]\n";

    ofs << "}\n";

    if (!ofs.good()) {
        m_lastError = "Write error on file: " + filepath;
        return false;
    }
    return true;
}

// ── Phase 11: loadSnapshot (with RegimeDetector + PortfolioAllocator Phi) ─────

bool StateSerializer::loadSnapshot(PortfolioManager&   portfolio,
                                   RiskEngine&         riskEngine,
                                   RegimeDetector&     regimeDetector,
                                   PortfolioAllocator& allocator,
                                   uint64_t&           checkpointTs,
                                   const std::string&  filepath) const
{
    // First load portfolio + riskEngine using the existing legacy path
    if (!loadSnapshot(portfolio, riskEngine, checkpointTs, filepath)) {
        return false;
    }

    // Now parse the extra Phase 11 sections from the file
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        m_lastError = "Cannot open file for reading: " + filepath;
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());

    ParseCtx ctx;
    ctx.p   = content.c_str();
    ctx.end = ctx.p + content.size();

    // ── RegimeDetector ────────────────────────────────────────────────────
    {
        ParseCtx tmp = ctx;
        if (scanKey(tmp, "regimeDetector")) {
            skipWs(tmp);
            if (tmp.p < tmp.end && *tmp.p == '{') {
                ++tmp.p;
                RegimeDetector::State rs;
                for (int fi = 0; fi < 14; ++fi) {
                    skipWs(tmp);
                    if (tmp.p < tmp.end && *tmp.p == '}') break;
                    std::string key;
                    if (!parseString(tmp, key)) break;
                    skipWs(tmp);
                    if (tmp.p < tmp.end && *tmp.p == ':') { ++tmp.p; }
                    if      (key == "smoothTR")        { parseDouble(tmp, rs.smoothTR); }
                    else if (key == "smoothDMPlus")    { parseDouble(tmp, rs.smoothDMPlus); }
                    else if (key == "smoothDMMinus")   { parseDouble(tmp, rs.smoothDMMinus); }
                    else if (key == "adx")             { parseDouble(tmp, rs.adx); }
                    else if (key == "adxObservations") { parseInt(tmp, rs.adxObservations); }
                    else if (key == "prevClose")       { parseDouble(tmp, rs.prevClose); }
                    else if (key == "prevCloseValid")  { parseBool(tmp, rs.prevCloseValid); }
                    else if (key == "variance")        { parseDouble(tmp, rs.variance); }
                    else if (key == "prevHigh")        { parseDouble(tmp, rs.prevHigh); }
                    else if (key == "prevLow")         { parseDouble(tmp, rs.prevLow); }
                    else if (key == "prevHLValid")     { parseBool(tmp, rs.prevHLValid); }
                    else if (key == "regime") {
                        int r = 2; parseInt(tmp, r);
                        rs.regime = static_cast<MarketRegime>(r);
                    }
                    else if (key == "returnWindow") {
                        std::vector<double> rv;
                        parseDoubleArray(tmp, rv);
                        rs.returnWindow.assign(rv.begin(), rv.end());
                    }
                    skipWs(tmp);
                    if (tmp.p < tmp.end && *tmp.p == ',') { ++tmp.p; }
                }
                regimeDetector.restoreState(rs);
            }
        }
    }

    // ── PortfolioAllocator Phi ────────────────────────────────────────────
    {
        ParseCtx tmp = ctx;
        if (scanKey(tmp, "allocatorPhi")) {
            skipWs(tmp);
            if (tmp.p < tmp.end && *tmp.p == '[') {
                ++tmp.p;
                std::unordered_map<std::string, PortfolioAllocator::PhiState> phiMap;
                skipWs(tmp);
                while (tmp.p < tmp.end && *tmp.p != ']') {
                    skipWs(tmp);
                    if (*tmp.p != '{') break;
                    ++tmp.p;
                    PortfolioAllocator::PhiState ps;
                    for (int fi = 0; fi < 4; ++fi) {
                        skipWs(tmp);
                        if (tmp.p < tmp.end && *tmp.p == '}') break;
                        std::string key;
                        if (!parseString(tmp, key)) break;
                        skipWs(tmp);
                        if (tmp.p < tmp.end && *tmp.p == ':') { ++tmp.p; }
                        if (key == "strategyId") { parseString(tmp, ps.strategyId); }
                        else if (key == "phi")   { parseDouble(tmp, ps.phi); }
                        else if (key == "pnlWindow") {
                            std::vector<double> rv;
                            parseDoubleArray(tmp, rv);
                            ps.pnlWindow.assign(rv.begin(), rv.end());
                        }
                        skipWs(tmp);
                        if (tmp.p < tmp.end && *tmp.p == ',') { ++tmp.p; }
                    }
                    skipWs(tmp);
                    if (tmp.p < tmp.end && *tmp.p == '}') { ++tmp.p; }
                    if (!ps.strategyId.empty()) { phiMap[ps.strategyId] = ps; }
                    skipWs(tmp);
                    if (tmp.p < tmp.end && *tmp.p == ',') { ++tmp.p; }
                    skipWs(tmp);
                }
                allocator.restorePhiStates(phiMap);
            }
        }
    }

    return true;
}
