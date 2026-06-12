#pragma once

#include "L2OrderBook.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class TriggerOrderType : uint8_t {
    STOP_LOSS = 0,
    OCO       = 1
};

enum class TriggerReason : uint8_t {
    NONE         = 0,
    STOP_TRIGGER = 1,
    TAKE_PROFIT  = 2
};

struct TriggerFillEvent {
    uint64_t      orderId{0};
    std::string   symbol;
    TriggerOrderType orderType{TriggerOrderType::STOP_LOSS};
    TriggerReason triggerReason{TriggerReason::NONE};
    bool          isBuy{false};
    double        triggerPrice{0.0};
    double        executionPrice{0.0};
    double        quantity{0.0};
    uint64_t      timestamp{0};
    std::string   strategyId;
};

class TriggerOrderManager {
public:
    explicit TriggerOrderManager(std::size_t capacity = 4096);

    uint64_t placeStopLoss(const std::string& symbol,
                           bool               isBuy,
                           double             stopTriggerPrice,
                           double             quantity = 0.0,
                           const std::string& strategyId = "") noexcept;

    uint64_t placeOco(const std::string& symbol,
                      bool               isBuy,
                      double             stopTriggerPrice,
                      double             takeProfitPrice,
                      double             quantity = 0.0,
                      const std::string& strategyId = "") noexcept;

    bool cancel(uint64_t orderId) noexcept;
    bool hasOrder(uint64_t orderId) const noexcept;

    std::size_t evaluate(const std::string& symbol,
                         const L2OrderBook::BestQuote& bbo,
                         uint64_t timestamp,
                         std::vector<TriggerFillEvent>& outFills);

    std::size_t activeOrderCount() const noexcept;
    std::size_t capacity() const noexcept;

private:
    struct OrderSlot {
        bool         inUse{false};
        uint64_t     orderId{0};
        std::string  symbol;
        TriggerOrderType orderType{TriggerOrderType::STOP_LOSS};
        bool         isBuy{false};
        double       stopPrice{0.0};
        double       takeProfitPrice{0.0};
        double       quantity{0.0};
        uint64_t     placedTs{0};
        std::string  strategyId;
    };

    uint64_t placeOrder(const std::string& symbol,
                        TriggerOrderType   type,
                        bool               isBuy,
                        double             stopPrice,
                        double             takeProfitPrice,
                        double             quantity,
                        const std::string& strategyId) noexcept;

    void releaseSlot(std::size_t idx) noexcept;

    std::vector<OrderSlot> m_slots;
    std::vector<std::size_t> m_freeIdx;
    uint64_t m_nextOrderId{1};
    std::size_t m_activeCount{0};
};
