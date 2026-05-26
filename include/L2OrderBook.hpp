#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

class L2OrderBook {
public:
    struct BestQuote {
        bool   valid{false};
        double bidPrice{0.0};
        double bidSize{0.0};
        double askPrice{0.0};
        double askSize{0.0};
    };

    explicit L2OrderBook(double tickSize = 0.01,
                         std::size_t levelCount = 262144);

    void reset(double referencePrice = 0.0) noexcept;

    bool applyBidLevel(double price, double quantity) noexcept;
    bool applyAskLevel(double price, double quantity) noexcept;
    bool applyBbo(double bidPrice, double bidQty,
                  double askPrice, double askQty) noexcept;

    BestQuote bbo() const noexcept;
    bool hasValidBbo() const noexcept;

    double tickSize() const noexcept;
    std::size_t levelCount() const noexcept;
    uint64_t recenterCount() const noexcept;

private:
    bool ensureMappable(double price) noexcept;
    std::size_t toIndex(double price) const noexcept;
    double toPrice(std::size_t idx) const noexcept;

    void recenter(double referencePrice) noexcept;
    void refreshBestBidFrom(std::size_t startIdx) noexcept;
    void refreshBestAskFrom(std::size_t startIdx) noexcept;

    double m_tickSize{0.01};
    std::vector<double> m_bidLevels;
    std::vector<double> m_askLevels;

    double m_basePrice{0.0};
    bool   m_initialized{false};

    int64_t m_bestBidIdx{-1};
    int64_t m_bestAskIdx{-1};
    int64_t m_lastBboBidIdx{-1};
    int64_t m_lastBboAskIdx{-1};
    uint64_t m_recenters{0};
};
