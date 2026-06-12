#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <optional>

// SPSC (single-producer, single-consumer) lock-free ring buffer.
// Capacity must be > 1 and is fixed at compile time.
template <typename T, std::size_t Capacity>
class LockFreeRingBuffer {
    static_assert(Capacity > 1, "LockFreeRingBuffer capacity must be > 1");

public:
    bool push(const T& value) noexcept
    {
        const auto head = m_head.load(std::memory_order_relaxed);
        const auto next = increment(head);
        if (next == m_tail.load(std::memory_order_acquire)) {
            return false; // full
        }
        m_data[head] = value;
        m_head.store(next, std::memory_order_release);
        return true;
    }

    bool pop(T& out) noexcept
    {
        const auto tail = m_tail.load(std::memory_order_relaxed);
        if (tail == m_head.load(std::memory_order_acquire)) {
            return false; // empty
        }
        out = m_data[tail];
        m_tail.store(increment(tail), std::memory_order_release);
        return true;
    }

    std::optional<T> pop() noexcept
    {
        T value{};
        if (!pop(value)) {
            return std::nullopt;
        }
        return value;
    }

    std::size_t size() const noexcept
    {
        const auto head = m_head.load(std::memory_order_acquire);
        const auto tail = m_tail.load(std::memory_order_acquire);
        if (head >= tail) {
            return head - tail;
        }
        return Capacity - (tail - head);
    }

private:
    static constexpr std::size_t increment(std::size_t idx) noexcept
    {
        return (idx + 1) % Capacity;
    }

    std::array<T, Capacity> m_data{};
    std::atomic<std::size_t> m_head{0};
    std::atomic<std::size_t> m_tail{0};
};
