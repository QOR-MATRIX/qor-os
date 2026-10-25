// SPDX-License-Identifier: LicenseRef-QTOS-Proprietary
#pragma once

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>

namespace qtos::audio {

// Wait-free single-producer, single-consumer queue of fixed capacity, for
// talking to the audio thread without locks.
template<typename T, std::size_t Capacity>
class SpscQueue
{
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

public:
    // Producer. Returns false when full.
    bool push(const T &value)
    {
        const std::size_t head = m_head.load(std::memory_order_relaxed);
        if (head - m_tail.load(std::memory_order_acquire) == Capacity)
            return false;
        m_items[head & (Capacity - 1)] = value;
        m_head.store(head + 1, std::memory_order_release);
        return true;
    }

    // Consumer.
    const T *peek() const
    {
        const std::size_t tail = m_tail.load(std::memory_order_relaxed);
        if (tail == m_head.load(std::memory_order_acquire))
            return nullptr;
        return &m_items[tail & (Capacity - 1)];
    }
    void pop() { m_tail.store(m_tail.load(std::memory_order_relaxed) + 1, std::memory_order_release); }
    std::optional<T> take()
    {
        const T *item = peek();
        if (!item)
            return std::nullopt;
        T value = *item;
        pop();
        return value;
    }

private:
    std::array<T, Capacity> m_items {};
    std::atomic<std::size_t> m_head { 0 };
    std::atomic<std::size_t> m_tail { 0 };
};

} // namespace qtos::audio
