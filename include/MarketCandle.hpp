#pragma once
#include <cstdint>
#include <string>

struct MarketCandle {
    std::string date;    // Raw date string, format: YYYY.MM.DD
    std::string time;    // Raw time string, format: HH:MM
    std::string symbol;  // Instrument identifier, e.g. "BTCUSDT"
    uint64_t    epochTimestamp{0}; // UTC Unix timestamp (seconds since epoch)
    double open;
    double high;
    double low;
    double close;
    double volume;
};
