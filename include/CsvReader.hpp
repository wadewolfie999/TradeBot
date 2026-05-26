#pragma once
#include "MarketCandle.hpp"
#include <fstream>
#include <string>

class CsvReader {
public:
    // `symbol` is stamped onto every MarketCandle read from this file.
    explicit CsvReader(const std::string& filepath,
                       std::string        symbol = "");

    // Returns true and populates candle on success.
    // Returns false on EOF or malformed line; sets internal error flag on malformed.
    bool readNextCandle(MarketCandle& candle);

    // Returns true if a malformed line or read error was encountered.
    bool hasError() const noexcept;

    // Returns true if the file was successfully opened.
    bool isOpen() const noexcept;

private:
    std::ifstream m_file;
    std::string   m_symbol;
    bool          m_error{false};
};
