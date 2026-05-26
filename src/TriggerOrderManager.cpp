#include "TriggerOrderManager.hpp"

#include <algorithm>

TriggerOrderManager::TriggerOrderManager(std::size_t capacity)
    : m_slots(capacity)
{
    m_freeIdx.reserve(capacity);
    for (std::size_t i = 0; i < capacity; ++i) {
        m_freeIdx.push_back(capacity - 1 - i);
    }
}

uint64_t TriggerOrderManager::placeStopLoss(const std::string& symbol,
                                            bool               isBuy,
                                            double             stopTriggerPrice,
                                            double             quantity,
                                            const std::string& strategyId) noexcept
{
    return placeOrder(symbol,
                      TriggerOrderType::STOP_LOSS,
                      isBuy,
                      stopTriggerPrice,
                      0.0,
                      quantity,
                      strategyId);
}

uint64_t TriggerOrderManager::placeOco(const std::string& symbol,
                                       bool               isBuy,
                                       double             stopTriggerPrice,
                                       double             takeProfitPrice,
                                       double             quantity,
                                       const std::string& strategyId) noexcept
{
    return placeOrder(symbol,
                      TriggerOrderType::OCO,
                      isBuy,
                      stopTriggerPrice,
                      takeProfitPrice,
                      quantity,
                      strategyId);
}

bool TriggerOrderManager::cancel(uint64_t orderId) noexcept
{
    for (std::size_t i = 0; i < m_slots.size(); ++i) {
        auto& slot = m_slots[i];
        if (!slot.inUse || slot.orderId != orderId) {
            continue;
        }
        releaseSlot(i);
        return true;
    }
    return false;
}

bool TriggerOrderManager::hasOrder(uint64_t orderId) const noexcept
{
    for (const auto& slot : m_slots) {
        if (slot.inUse && slot.orderId == orderId) {
            return true;
        }
    }
    return false;
}

std::size_t TriggerOrderManager::evaluate(const std::string& symbol,
                                          const L2OrderBook::BestQuote& bbo,
                                          uint64_t timestamp,
                                          std::vector<TriggerFillEvent>& outFills)
{
    if (!bbo.valid) {
        return 0;
    }

    std::size_t filled = 0;
    for (std::size_t i = 0; i < m_slots.size(); ++i) {
        auto& slot = m_slots[i];
        if (!slot.inUse || slot.symbol != symbol) {
            continue;
        }

        TriggerReason reason = TriggerReason::NONE;
        double triggerPrice = 0.0;

        const bool sellSide = !slot.isBuy;
        if (slot.orderType == TriggerOrderType::STOP_LOSS) {
            if (sellSide) {
                if (bbo.bidPrice <= slot.stopPrice) {
                    reason = TriggerReason::STOP_TRIGGER;
                    triggerPrice = slot.stopPrice;
                }
            } else {
                if (bbo.askPrice >= slot.stopPrice) {
                    reason = TriggerReason::STOP_TRIGGER;
                    triggerPrice = slot.stopPrice;
                }
            }
        } else if (slot.orderType == TriggerOrderType::OCO) {
            if (sellSide) {
                if (bbo.bidPrice <= slot.stopPrice) {
                    reason = TriggerReason::STOP_TRIGGER;
                    triggerPrice = slot.stopPrice;
                } else if (bbo.askPrice >= slot.takeProfitPrice) {
                    reason = TriggerReason::TAKE_PROFIT;
                    triggerPrice = slot.takeProfitPrice;
                }
            } else {
                if (bbo.askPrice >= slot.stopPrice) {
                    reason = TriggerReason::STOP_TRIGGER;
                    triggerPrice = slot.stopPrice;
                } else if (bbo.bidPrice <= slot.takeProfitPrice) {
                    reason = TriggerReason::TAKE_PROFIT;
                    triggerPrice = slot.takeProfitPrice;
                }
            }
        }

        if (reason == TriggerReason::NONE) {
            continue;
        }

        TriggerFillEvent ev;
        ev.orderId = slot.orderId;
        ev.symbol = slot.symbol;
        ev.orderType = slot.orderType;
        ev.triggerReason = reason;
        ev.isBuy = slot.isBuy;
        ev.triggerPrice = triggerPrice;
        ev.executionPrice = slot.isBuy ? bbo.askPrice : bbo.bidPrice;
        ev.quantity = slot.quantity;
        ev.timestamp = timestamp;
        ev.strategyId = slot.strategyId;
        outFills.push_back(std::move(ev));

        releaseSlot(i);
        ++filled;
    }

    return filled;
}

std::size_t TriggerOrderManager::activeOrderCount() const noexcept
{
    return m_activeCount;
}

std::size_t TriggerOrderManager::capacity() const noexcept
{
    return m_slots.size();
}

uint64_t TriggerOrderManager::placeOrder(const std::string& symbol,
                                         TriggerOrderType   type,
                                         bool               isBuy,
                                         double             stopPrice,
                                         double             takeProfitPrice,
                                         double             quantity,
                                         const std::string& strategyId) noexcept
{
    if (symbol.empty() || stopPrice <= 0.0) {
        return 0;
    }
    if (type == TriggerOrderType::OCO && takeProfitPrice <= 0.0) {
        return 0;
    }
    if (m_freeIdx.empty()) {
        return 0;
    }

    const std::size_t idx = m_freeIdx.back();
    m_freeIdx.pop_back();

    auto& slot = m_slots[idx];
    slot.inUse = true;
    slot.orderId = m_nextOrderId++;
    slot.symbol = symbol;
    slot.orderType = type;
    slot.isBuy = isBuy;
    slot.stopPrice = stopPrice;
    slot.takeProfitPrice = takeProfitPrice;
    slot.quantity = quantity;
    slot.strategyId = strategyId;

    ++m_activeCount;
    return slot.orderId;
}

void TriggerOrderManager::releaseSlot(std::size_t idx) noexcept
{
    if (idx >= m_slots.size()) {
        return;
    }
    auto& slot = m_slots[idx];
    if (!slot.inUse) {
        return;
    }

    slot = OrderSlot{};
    m_freeIdx.push_back(idx);
    if (m_activeCount > 0) {
        --m_activeCount;
    }
}
