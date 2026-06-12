#include "CsvReader.hpp"

#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>

CsvReader::CsvReader(const std::string& filepath, std::string symbol)
    : m_file(filepath)
    , m_symbol(std::move(symbol))
{
    if (m_file.is_open()) {
        m_file.imbue(std::locale::classic());
    }
}

bool CsvReader::isOpen() const noexcept
{
    return m_file.is_open();
}

bool CsvReader::hasError() const noexcept
{
    return m_error;
}

// Helper: parse a double from a string using the classic locale.
static bool parseDouble(const std::string& token, double& out)
{
    if (token.empty()) return false;
    std::istringstream ss(token);
    ss.imbue(std::locale::classic());
    ss >> out;
    return !ss.fail();
}

// Helper: strip a trailing '\r' if present (CRLF files).
static void stripCR(std::string& s)
{
    if (!s.empty() && s.back() == '\r') {
        s.pop_back();
    }
}

// Helper: parse "YYYY.MM.DD" and "HH:MM" into a UTC Unix timestamp (seconds
// since epoch).  Uses timegm (POSIX/glibc/macOS) or _mkgmtime (MSVC) to
// convert a broken-down UTC time without local-timezone bleed.
// Returns 0 on parse failure.
static uint64_t parseEpoch(const std::string& dateStr, const std::string& timeStr)
{
    std::string combined = dateStr + " " + timeStr;
    std::istringstream ss(combined);
    ss.imbue(std::locale::classic());

    std::tm t{};
    ss >> std::get_time(&t, "%Y.%m.%d %H:%M");
    if (ss.fail()) {
        return 0;
    }

    t.tm_sec  = 0;
    t.tm_isdst = 0;

#if defined(_WIN32)
    std::time_t epoch = _mkgmtime(&t);
#else
    std::time_t epoch = timegm(&t);
#endif

    if (epoch == static_cast<std::time_t>(-1)) {
        return 0;
    }
    return static_cast<uint64_t>(epoch);
}

bool CsvReader::readNextCandle(MarketCandle& candle)
{
    std::string line;
    while (std::getline(m_file, line)) {
        stripCR(line);
        if (line.empty()) {
            continue; // skip blank lines without flagging an error
        }

        // Tokenise on commas.
        std::string tokens[7];
        int tokenIdx = 0;
        std::istringstream ss(line);
        std::string token;
        while (tokenIdx < 7 && std::getline(ss, token, ',')) {
            tokens[tokenIdx++] = token;
        }

        if (tokenIdx != 7) {
            m_error = true;
            return false;
        }

        double open{}, high{}, low{}, close{}, volume{};
        if (!parseDouble(tokens[2], open)   ||
            !parseDouble(tokens[3], high)   ||
            !parseDouble(tokens[4], low)    ||
            !parseDouble(tokens[5], close)  ||
            !parseDouble(tokens[6], volume)) {
            m_error = true;
            return false;
        }

        candle.date           = tokens[0];
        candle.time           = tokens[1];
        candle.epochTimestamp = parseEpoch(tokens[0], tokens[1]);
        candle.symbol         = m_symbol;
        candle.open           = open;
        candle.high           = high;
        candle.low            = low;
        candle.close          = close;
        candle.volume         = volume;
        return true;
    }

    return false; // EOF - not an error
}
