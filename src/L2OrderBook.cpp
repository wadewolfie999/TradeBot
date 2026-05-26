#include "L2OrderBook.hpp"

#include <algorithm>
#include <cmath>

L2OrderBook::L2OrderBook(double tickSize, std::size_t levelCount)
    : m_tickSize((tickSize > 0.0) ? tickSize : 0.01)
    , m_bidLevels(levelCount, 0.0)
    , m_askLevels(levelCount, 0.0)
{}

void L2OrderBook::reset(double referencePrice) noexcept
{
    std::fill(m_bidLevels.begin(), m_bidLevels.end(), 0.0);
    std::fill(m_askLevels.begin(), m_askLevels.end(), 0.0);
    m_bestBidIdx = -1;
    m_bestAskIdx = -1;
    m_lastBboBidIdx = -1;
    m_lastBboAskIdx = -1;

    if (referencePrice > 0.0) {
        recenter(referencePrice);
    } else {
        m_basePrice = 0.0;
        m_initialized = false;
    }
}

bool L2OrderBook::applyBidLevel(double price, double quantity) noexcept
{
    if (price <= 0.0 || quantity < 0.0) {
        return false;
    }
    if (!ensureMappable(price)) {
        return false;
    }

    const std::size_t idx = toIndex(price);
    m_bidLevels[idx] = quantity;

    if (quantity > 0.0) {
        if (m_bestBidIdx < 0 || static_cast<std::size_t>(m_bestBidIdx) < idx) {
            m_bestBidIdx = static_cast<int64_t>(idx);
        }
    } else if (m_bestBidIdx == static_cast<int64_t>(idx)) {
        refreshBestBidFrom(idx);
    }

    return true;
}

bool L2OrderBook::applyAskLevel(double price, double quantity) noexcept
{
    if (price <= 0.0 || quantity < 0.0) {
        return false;
    }
    if (!ensureMappable(price)) {
        return false;
    }

    const std::size_t idx = toIndex(price);
    m_askLevels[idx] = quantity;

    if (quantity > 0.0) {
        if (m_bestAskIdx < 0 || static_cast<std::size_t>(m_bestAskIdx) > idx) {
            m_bestAskIdx = static_cast<int64_t>(idx);
        }
    } else if (m_bestAskIdx == static_cast<int64_t>(idx)) {
        refreshBestAskFrom(idx);
    }

    return true;
}

bool L2OrderBook::applyBbo(double bidPrice, double bidQty, double askPrice, double askQty) noexcept
{
    if (bidPrice <= 0.0 || askPrice <= 0.0 || askPrice < bidPrice) {
        return false;
    }

    if (!ensureMappable(bidPrice) || !ensureMappable(askPrice)) {
        return false;
    }

    if (m_lastBboBidIdx >= 0) {
        const auto idx = static_cast<std::size_t>(m_lastBboBidIdx);
        if (idx < m_bidLevels.size()) {
            m_bidLevels[idx] = 0.0;
            if (m_bestBidIdx == m_lastBboBidIdx) {
                refreshBestBidFrom(idx);
            }
        }
    }
    if (m_lastBboAskIdx >= 0) {
        const auto idx = static_cast<std::size_t>(m_lastBboAskIdx);
        if (idx < m_askLevels.size()) {
            m_askLevels[idx] = 0.0;
            if (m_bestAskIdx == m_lastBboAskIdx) {
                refreshBestAskFrom(idx);
            }
        }
    }

    const bool okBid = applyBidLevel(bidPrice, std::max(0.0, bidQty));
    const bool okAsk = applyAskLevel(askPrice, std::max(0.0, askQty));
    if (okBid) {
        m_lastBboBidIdx = static_cast<int64_t>(toIndex(bidPrice));
    }
    if (okAsk) {
        m_lastBboAskIdx = static_cast<int64_t>(toIndex(askPrice));
    }
    return okBid && okAsk;
}

L2OrderBook::BestQuote L2OrderBook::bbo() const noexcept
{
    BestQuote q;
    if (m_bestBidIdx < 0 || m_bestAskIdx < 0) {
        q.valid = false;
        return q;
    }
    const auto bidIdx = static_cast<std::size_t>(m_bestBidIdx);
    const auto askIdx = static_cast<std::size_t>(m_bestAskIdx);
    if (bidIdx >= m_bidLevels.size() || askIdx >= m_askLevels.size()) {
        q.valid = false;
        return q;
    }
    q.valid = true;
    q.bidPrice = toPrice(bidIdx);
    q.bidSize  = m_bidLevels[bidIdx];
    q.askPrice = toPrice(askIdx);
    q.askSize  = m_askLevels[askIdx];
    return q;
}

bool L2OrderBook::hasValidBbo() const noexcept
{
    return bbo().valid;
}

double L2OrderBook::tickSize() const noexcept
{
    return m_tickSize;
}

std::size_t L2OrderBook::levelCount() const noexcept
{
    return m_bidLevels.size();
}

uint64_t L2OrderBook::recenterCount() const noexcept
{
    return m_recenters;
}

bool L2OrderBook::ensureMappable(double price) noexcept
{
    if (price <= 0.0) {
        return false;
    }
    if (!m_initialized) {
        recenter(price);
        return true;
    }

    const double maxDistanceTicks =
        static_cast<double>(m_bidLevels.size() / 4);
    const double diffTicks = std::fabs(price - m_basePrice) / m_tickSize;
    if (diffTicks > maxDistanceTicks) {
        recenter(price);
    }
    return true;
}

std::size_t L2OrderBook::toIndex(double price) const noexcept
{
    if (price <= 0.0 || m_tickSize <= 0.0) {
        return 0;
    }
    const double ticks = std::floor((price - m_basePrice) / m_tickSize + 0.5);
    const auto mid = m_bidLevels.size() / 2;
    const auto idx = static_cast<std::size_t>(
        std::max<int64_t>(0, std::min<int64_t>(
            static_cast<int64_t>(mid + static_cast<int64_t>(ticks)),
            static_cast<int64_t>(m_bidLevels.size() - 1))));
    return idx;
}

double L2OrderBook::toPrice(std::size_t idx) const noexcept
{
    const auto mid = m_bidLevels.size() / 2;
    const int64_t ticks = static_cast<int64_t>(idx) -
                          static_cast<int64_t>(mid);
    return m_basePrice + static_cast<double>(ticks) * m_tickSize;
}

void L2OrderBook::recenter(double referencePrice) noexcept
{
    if (referencePrice <= 0.0 || m_tickSize <= 0.0) {
        return;
    }
    m_basePrice = referencePrice;
    m_initialized = true;
    m_bestBidIdx = -1;
    m_bestAskIdx = -1;
    m_lastBboBidIdx = -1;
    m_lastBboAskIdx = -1;
    std::fill(m_bidLevels.begin(), m_bidLevels.end(), 0.0);
    std::fill(m_askLevels.begin(), m_askLevels.end(), 0.0);
    ++m_recenters;
}

void L2OrderBook::refreshBestBidFrom(std::size_t startIdx) noexcept
{
    if (startIdx >= m_bidLevels.size()) {
        m_bestBidIdx = -1;
        return;
    }
    for (std::size_t i = startIdx; i-- > 0;) {
        if (m_bidLevels[i] > 0.0) {
            m_bestBidIdx = static_cast<int64_t>(i);
            return;
        }
    }
    m_bestBidIdx = -1;
}

void L2OrderBook::refreshBestAskFrom(std::size_t startIdx) noexcept
{
    if (startIdx >= m_askLevels.size()) {
        m_bestAskIdx = -1;
        return;
    }
    for (std::size_t i = startIdx; i < m_askLevels.size(); ++i) {
        if (m_askLevels[i] > 0.0) {
            m_bestAskIdx = static_cast<int64_t>(i);
            return;
        }
    }
    m_bestAskIdx = -1;
}
