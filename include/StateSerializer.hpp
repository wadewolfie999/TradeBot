#pragma once
#include "PortfolioManager.hpp"
#include "RiskEngine.hpp"
#include "RegimeDetector.hpp"
#include "PortfolioAllocator.hpp"
#include <string>
#include <cstdint>

// ── StateSerializer ───────────────────────────────────────────────────────────
// Serializes / deserializes the mutable engine state to a JSON text file.
//
// Version 11 snapshot adds two top-level sections:
//   "regimeDetector": { ADX/variance rolling buffers }
//   "allocatorPhi":   [ { strategy_id, pnlWindow, phi } ]
//
// All floating-point values written with 17 significant digits for exact
// round-trip fidelity (IEEE 754 double).
// ─────────────────────────────────────────────────────────────────────────────
class StateSerializer {
public:
    // Save full snapshot (Phase 11: includes regimeDetector + allocatorPhi).
    // Returns true on success.
    bool saveSnapshot(const PortfolioManager&  portfolio,
                      const RiskEngine&        riskEngine,
                      const RegimeDetector&    regimeDetector,
                      const PortfolioAllocator& allocator,
                      uint64_t                 checkpointTs,
                      const std::string&       filepath = "data/results/snapshot.json") const;

    // Legacy save (Phase 9 / 10 compat) — no regime / allocator state.
    bool saveSnapshot(const PortfolioManager& portfolio,
                      const RiskEngine&       riskEngine,
                      uint64_t                checkpointTs,
                      const std::string&      filepath = "data/results/snapshot.json") const;

    // Load full snapshot (Phase 11).
    bool loadSnapshot(PortfolioManager&   portfolio,
                      RiskEngine&         riskEngine,
                      RegimeDetector&     regimeDetector,
                      PortfolioAllocator& allocator,
                      uint64_t&           checkpointTs,
                      const std::string&  filepath = "data/results/snapshot.json") const;

    // Legacy load (Phase 9 / 10 compat).
    bool loadSnapshot(PortfolioManager& portfolio,
                      RiskEngine&       riskEngine,
                      uint64_t&         checkpointTs,
                      const std::string& filepath = "data/results/snapshot.json") const;

    const std::string& lastError() const noexcept;

private:
    mutable std::string m_lastError;

    static std::string writeDouble(double v);
    static std::string writeUint64(uint64_t v);
    static std::string writeBool(bool v);

    struct ParseCtx {
        const char* p{nullptr};
        const char* end{nullptr};
        std::string error;
    };
    static void skipWs(ParseCtx& ctx);
    static bool expect(ParseCtx& ctx, char c);
    static bool parseString(ParseCtx& ctx, std::string& out);
    static bool parseDouble(ParseCtx& ctx, double& out);
    static bool parseUint64(ParseCtx& ctx, uint64_t& out);
    static bool parseInt(ParseCtx& ctx, int& out);
    static bool parseBool(ParseCtx& ctx, bool& out);
    static bool parseDoubleArray(ParseCtx& ctx, std::vector<double>& out);
    static bool parseStringArray(ParseCtx& ctx, std::vector<std::string>& out);
    static bool scanKey(ParseCtx& ctx, const std::string& key);

    // Shared helpers used by both save overloads
    bool savePortfolioSection(std::ofstream& ofs,
                              const PortfolioManager& portfolio) const;
    bool saveRiskEngineSection(std::ofstream& ofs,
                               const RiskEngine& riskEngine) const;

    // Shared helpers used by both load overloads
    bool loadPortfolioSection(ParseCtx& ctx, PortfolioManager& portfolio) const;
    bool loadRiskEngineSection(ParseCtx& ctx, RiskEngine& riskEngine) const;
};
