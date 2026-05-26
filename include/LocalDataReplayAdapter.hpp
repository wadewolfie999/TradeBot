#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

enum class ReplaySpeedMode : uint8_t {
    REALTIME_1X = 0,
    FAST_10X    = 1,
    MAX_EFFORT  = 2
};

struct ReplayTick {
    uint64_t    timestampNs{0};
    std::string symbol;
    double      bidPrice{0.0};
    double      bidSize{0.0};
    double      askPrice{0.0};
    double      askSize{0.0};
    double      tradePrice{0.0};
    double      tradeSize{0.0};
    uint8_t     flags{0};
};

class LocalDataReplayAdapter {
public:
    LocalDataReplayAdapter() = default;

    bool loadFromPath(const std::string& path,
                      const std::string& defaultSymbol = "");
    bool loadCsv(const std::string& path,
                 const std::string& defaultSymbol = "");
    bool loadBinary(const std::string& path,
                    const std::string& defaultSymbol = "");

    static bool writeBinary(const std::string& path,
                            const std::vector<ReplayTick>& ticks,
                            std::string* err = nullptr);

    void setSpeedMode(ReplaySpeedMode mode) noexcept;
    void setSpeedFactor(double factor) noexcept;

    bool nextTick(ReplayTick& out);
    bool seek(std::size_t idx) noexcept;
    void resetCursor() noexcept;

    std::size_t size() const noexcept;
    std::size_t cursor() const noexcept;
    bool empty() const noexcept;

    std::string lastError() const;

private:
#pragma pack(push, 1)
    struct BinaryTickRecord {
        uint64_t timestampNs;
        double   bidPrice;
        double   bidSize;
        double   askPrice;
        double   askSize;
        double   tradePrice;
        double   tradeSize;
        uint8_t  flags;
        char     symbol[16];
    };
#pragma pack(pop)

    bool parseCsvLine(const std::string& line,
                      const std::string& defaultSymbol,
                      ReplayTick& out) const;
    static bool hasBinaryExtension(const std::string& path) noexcept;

    void clearPacing() noexcept;
    uint64_t computeReplayDelayNs(const ReplayTick& prev,
                                  const ReplayTick& cur) const noexcept;

    void setLastError(const std::string& msg);

    std::vector<ReplayTick> m_ticks;
    std::size_t m_cursor{0};

    ReplaySpeedMode m_speedMode{ReplaySpeedMode::REALTIME_1X};
    double m_speedFactor{1.0};

    bool m_hasLastWallClock{false};
    std::chrono::steady_clock::time_point m_lastWallClock{};
    ReplayTick m_lastReplayTick{};

    mutable std::mutex m_errorMutex;
    std::string m_lastError;
};
