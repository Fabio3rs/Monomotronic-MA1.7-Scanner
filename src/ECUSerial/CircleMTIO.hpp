#pragma once
#ifndef CIRCLEMTIO_HPP
#define CIRCLEMTIO_HPP

/*
 * CircleMTIO - Fixed-Size Circular Buffer for Multi-Threaded I/O
 *
 * NASA Power of 10 Compliant:
 * - Fixed-size allocation at compile-time (no dynamic memory)
 * - Lock-free operation for common case (mutex only at wrap-around)
 * - Cache-line aligned to prevent false sharing
 * - Overrun tracking for observability
 *
 * Based on cppapiframework/src/utils/CircleMTIO.hpp
 * Enhanced for ECU scanner with overrun tracking
 */

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>

// Cache-line aligned wrapper to prevent false sharing between threads
template <class T> struct __attribute__((aligned(64))) singlevarnofsh {
    T a;
};

/**
 * @brief Fixed-size circular buffer for multi-threaded producer-consumer
 *
 * @tparam num Buffer capacity (fixed at compile-time)
 * @tparam T Element type
 *
 * Thread Safety:
 * - Single producer, single consumer safe
 * - Multiple producers with lock contention at wrap-around
 *
 * Element States:
 * - 0: Free (can be written)
 * - 1: Writing (reserved by writer)
 * - 2: Ready (available for reading)
 */
template <size_t num, class T> class __attribute__((aligned(64))) CircleMTIO {
    std::array<T, num> elements;
    std::array<singlevarnofsh<std::atomic<int>>, num> elements_state{};

    std::atomic<size_t> reading_point{};
    std::atomic<size_t> writing_point{};
    std::mutex mtx;

    // NASA P10: Observability - track buffer overruns
    std::atomic<uint64_t> overrun_count{0};
    std::atomic<uint64_t> total_writes{0};
    std::atomic<uint64_t> total_reads{0};

  public:
    using value_type = T;
    using pair_t = std::pair<value_type, size_t>;

    /**
     * @brief Reserve a slot for writing
     *
     * @return std::pair<T*, size_t> Pointer to writable element and its index
     *         Returns {nullptr, 0} if buffer is full (overrun condition)
     *
     * Usage:
     *   auto [ptr, idx] = buffer.new_write();
     *   if (ptr != nullptr) {
     *       *ptr = my_data;
     *       buffer.set_ready(idx);
     *   }
     */
    auto new_write() -> std::pair<T *, size_t> {
        size_t a = writing_point.fetch_add(1, std::memory_order_relaxed);

        // Handle wrap-around with mutex to prevent race
        if (a >= num) {
            std::lock_guard<std::mutex> lck(mtx);
            a = writing_point.fetch_add(1, std::memory_order_relaxed);

            if (a >= num) {
                a = 0;
                writing_point.store(1, std::memory_order_relaxed);
            }
        }

        // Check if slot is available (state must be 0 = free)
        if (elements_state[a].a.load(std::memory_order_acquire) != 0) {
            // Buffer full - overrun condition
            overrun_count.fetch_add(1, std::memory_order_relaxed);
            return std::pair<T *, size_t>(nullptr, 0);
        }

        // Mark as writing (state 1)
        elements_state[a].a.store(1, std::memory_order_release);
        total_writes.fetch_add(1, std::memory_order_relaxed);

        return std::pair<T *, size_t>(&elements[a], a);
    }

    /**
     * @brief Get next available element for reading
     *
     * @return std::pair<T*, size_t> Pointer to readable element and its index
     *         Returns {nullptr, ~0} if no data available
     *
     * Usage:
     *   while (true) {
     *       auto [ptr, idx] = buffer.next();
     *       if (ptr == nullptr) break;
     *       process(*ptr);
     *       buffer.set_free(idx);
     *   }
     */
    auto next() -> std::pair<T *, size_t> {
        size_t r = reading_point.load(std::memory_order_relaxed);

        // Check if element is ready (state must be 2)
        if (elements_state[r].a.load(std::memory_order_acquire) != 2) {
            return std::pair<T *, size_t>(nullptr, ~std::size_t(0));
        }

        // Check if we've caught up to writer
        if (r == writing_point.load(std::memory_order_relaxed)) {
            return std::pair<T *, size_t>(nullptr, ~std::size_t(0));
        }

        // Advance reading point
        reading_point.fetch_add(1, std::memory_order_relaxed);

        if (reading_point.load(std::memory_order_relaxed) >= num) {
            reading_point.store(0, std::memory_order_relaxed);
        }

        total_reads.fetch_add(1, std::memory_order_relaxed);

        return std::pair<T *, size_t>(&elements[r], r);
    }

    /**
     * @brief Mark element as free (after reading)
     * @param a Index returned by next()
     */
    void set_free(size_t a) {
        elements_state[a].a.store(0, std::memory_order_release);
    }

    /**
     * @brief Mark element as ready for reading (after writing)
     * @param a Index returned by new_write()
     */
    void set_ready(size_t a) {
        elements_state[a].a.store(2, std::memory_order_release);
    }

    /**
     * @brief Get buffer capacity
     * @return size_t Maximum number of elements
     */
    constexpr size_t capacity() const { return num; }

    /**
     * @brief Get total number of overruns (writes rejected due to full buffer)
     * @return uint64_t Overrun counter
     */
    uint64_t get_overruns() const {
        return overrun_count.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get total writes attempted (including overruns)
     * @return uint64_t Total write counter
     */
    uint64_t get_total_writes() const {
        return total_writes.load(std::memory_order_relaxed);
    }

    /**
     * @brief Get total reads completed
     * @return uint64_t Total read counter
     */
    uint64_t get_total_reads() const {
        return total_reads.load(std::memory_order_relaxed);
    }

    /**
     * @brief Reset overrun counter
     */
    void reset_overrun_count() {
        overrun_count.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief Reset all statistics counters
     */
    void reset_stats() {
        overrun_count.store(0, std::memory_order_relaxed);
        total_writes.store(0, std::memory_order_relaxed);
        total_reads.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief Get approximate number of elements available for reading
     * @return size_t Number of elements (approximate due to lock-free nature)
     */
    size_t available_count() const {
        size_t w = writing_point.load(std::memory_order_relaxed);
        size_t r = reading_point.load(std::memory_order_relaxed);

        if (w >= r) {
            return w - r;
        } else {
            return (num - r) + w;
        }
    }

    CircleMTIO() {
        reading_point.store(0, std::memory_order_relaxed);
        writing_point.store(0, std::memory_order_relaxed);
        overrun_count.store(0, std::memory_order_relaxed);
        total_writes.store(0, std::memory_order_relaxed);
        total_reads.store(0, std::memory_order_relaxed);

        // Initialize all elements to free state
        for (auto &b : elements_state) {
            b.a.store(0, std::memory_order_relaxed);
        }
    }
};

#endif // CIRCLEMTIO_HPP
