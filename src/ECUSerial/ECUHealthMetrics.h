#pragma once
#ifndef ECU_HEALTH_METRICS_H
#define ECU_HEALTH_METRICS_H

/*
 * ECUHealthMetrics - Health and Performance Metrics for ECU Communication
 *
 * NASA Power of 10 Compliant:
 * - Fixed-size structure (no dynamic allocation)
 * - Atomic counters for thread-safe updates
 * - Observability for real-time diagnostics
 *
 * Purpose: Track buffer overruns, latency, errors, and communication statistics
 * to validate NASA P10 compliance and real-time performance.
 */

#include <atomic>
#include <chrono>
#include <cstdint>

namespace ECU {

/**
 * @brief Snapshot of health metrics (non-atomic copy)
 *
 * This struct contains plain (non-atomic) copies of metrics
 * for safe reading without locking. Use HealthMetrics::snapshot()
 * to create a consistent snapshot.
 */
struct HealthMetricsSnapshot {
    // Buffer management
    uint64_t buffer_overruns{0};
    uint64_t packet_pool_exhaustions{0};

    // Performance
    uint64_t max_loop_latency_us{0};
    uint64_t avg_loop_latency_us{0};

    // Errors
    uint32_t consecutive_checksum_errors{0};
    uint32_t consecutive_timeout_errors{0};
    uint32_t total_recovery_cycles{0};

    // Legacy telemetry migration
    uint64_t total_ack_mismatches{0};
    uint64_t total_end_marker_errors{0};
    uint64_t total_retry_events{0};

    // Statistics
    uint64_t total_packets_sent{0};
    uint64_t total_packets_received{0};
    uint64_t total_bytes_sent{0};
    uint64_t total_bytes_received{0};

    // Timing
    std::chrono::steady_clock::time_point last_successful_communication;
    std::chrono::steady_clock::time_point thread_start_time;
};

/**
 * @brief Health and performance metrics for ECU communication thread
 *
 * All atomic members use relaxed memory ordering since these are
 * informational metrics, not synchronization primitives.
 *
 * Thread Safety:
 * - All atomic members can be updated from worker thread
 * - snapshot() provides atomic read for main thread
 * - Non-atomic members (timestamps) only written by worker thread
 */
struct HealthMetrics {
    // ========== Buffer Management ==========

    /// Number of times logging buffer wrapped around (overruns)
    /// Incremented when addbytelog() fails due to full buffer
    std::atomic<uint64_t> buffer_overruns{0};

    /// Number of times packet pool was exhausted (future use)
    /// Reserved for BorrowPool implementation if adopted
    std::atomic<uint64_t> packet_pool_exhaustions{0};

    // ========== Performance Tracking ==========

    /// Maximum observed loop latency in microseconds (State READY)
    /// Updated via compare-and-swap to track worst case
    std::atomic<uint64_t> max_loop_latency_us{0};

    /// Exponential moving average of loop latency in microseconds
    /// Formula: new_avg = (old_avg * 15 + current) / 16
    std::atomic<uint64_t> avg_loop_latency_us{0};

    // ========== Error Tracking ==========

    /// Consecutive checksum errors (end marker != 0x03)
    /// Reset to 0 on successful packet, triggers recovery at 3
    std::atomic<uint32_t> consecutive_checksum_errors{0};

    /// Consecutive timeout errors (no response in 1000ms)
    /// Reset to 0 on successful communication
    std::atomic<uint32_t> consecutive_timeout_errors{0};

    /// Total number of recovery cycles (State RECOVERING entered)
    /// Indicates connection instability
    std::atomic<uint32_t> total_recovery_cycles{0};

    // ========== Legacy Telemetry Migration ==========
    // NOTE: These duplicate existing telemetry_* counters in ECUMonomotronic
    // for backward compatibility. Will be consolidated in future version.

    /// Total ACK/echo mismatches (lifetime counter, never reset)
    /// Mirrors telemetry_ack_mismatch for observability
    std::atomic<uint64_t> total_ack_mismatches{0};

    /// Total end marker errors (lifetime counter, never reset)
    /// Mirrors telemetry_end_marker_error for observability
    std::atomic<uint64_t> total_end_marker_errors{0};

    /// Total retry events (lifetime counter, never reset)
    /// Mirrors telemetry_retry_events for observability
    std::atomic<uint64_t> total_retry_events{0};

    // ========== Communication Statistics ==========

    /// Total packets successfully sent (incremented in ECUWritePacket)
    std::atomic<uint64_t> total_packets_sent{0};

    /// Total packets successfully received (incremented in ECUReadPacket)
    std::atomic<uint64_t> total_packets_received{0};

    /// Total bytes sent (includes packet headers and end markers)
    std::atomic<uint64_t> total_bytes_sent{0};

    /// Total bytes received (includes packet headers and end markers)
    std::atomic<uint64_t> total_bytes_received{0};

    // ========== Timing Information ==========
    // Non-atomic: Only written by worker thread, safe to read from main thread

    /// Timestamp of last successful communication (packet sent/received)
    /// Used to detect stale connections
    std::chrono::steady_clock::time_point last_successful_communication;

    /// Timestamp when ECU thread started
    /// Used to calculate uptime and average rates
    std::chrono::steady_clock::time_point thread_start_time;

    // ========== Methods ==========

    /**
     * @brief Create atomic snapshot of all metrics
     * @return HealthMetricsSnapshot Copy with consistent values
     *
     * Thread-safe: Can be called from any thread while worker updates metrics
     */
    HealthMetricsSnapshot snapshot() const;

    /**
     * @brief Reset all metrics to initial state
     *
     * Thread-safe: Uses atomic stores with relaxed ordering
     * Note: Should only be called when worker thread is idle
     */
    void reset();

    /**
     * @brief Get uptime in seconds since thread start
     * @return double Seconds elapsed
     */
    double get_uptime_seconds() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - thread_start_time);
        return duration.count() / 1000.0;
    }

    /**
     * @brief Get time since last successful communication in milliseconds
     * @return int64_t Milliseconds elapsed, -1 if never communicated
     */
    int64_t get_ms_since_last_communication() const {
        if (last_successful_communication.time_since_epoch().count() == 0) {
            return -1; // Never communicated
        }

        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_successful_communication);
        return duration.count();
    }

    /**
     * @brief Get average packets per second
     * @return double Packets/second (sent + received)
     */
    double get_avg_packets_per_second() const {
        double uptime = get_uptime_seconds();
        if (uptime <= 0.0)
            return 0.0;

        uint64_t total_packets =
            total_packets_sent.load(std::memory_order_relaxed) +
            total_packets_received.load(std::memory_order_relaxed);

        return static_cast<double>(total_packets) / uptime;
    }

    /**
     * @brief Get average bytes per second
     * @return double Bytes/second (sent + received)
     */
    double get_avg_bytes_per_second() const {
        double uptime = get_uptime_seconds();
        if (uptime <= 0.0)
            return 0.0;

        uint64_t total_bytes =
            total_bytes_sent.load(std::memory_order_relaxed) +
            total_bytes_received.load(std::memory_order_relaxed);

        return static_cast<double>(total_bytes) / uptime;
    }

    /**
     * @brief Get packet error rate (recovery cycles / total packets)
     * @return double Error rate (0.0 to 1.0)
     */
    double get_error_rate() const {
        uint64_t total_packets =
            total_packets_sent.load(std::memory_order_relaxed) +
            total_packets_received.load(std::memory_order_relaxed);

        if (total_packets == 0)
            return 0.0;

        uint32_t recoveries =
            total_recovery_cycles.load(std::memory_order_relaxed);

        return static_cast<double>(recoveries) /
               static_cast<double>(total_packets);
    }

    /**
     * @brief Check if metrics indicate healthy operation
     * @return bool True if all indicators are good
     *
     * Criteria:
     * - No consecutive errors
     * - Recent communication (<5 seconds)
     * - Error rate <1%
     * - Buffer overruns <10% of writes
     */
    bool is_healthy() const {
        // Check for consecutive errors
        if (consecutive_checksum_errors.load(std::memory_order_relaxed) > 0) {
            return false;
        }

        if (consecutive_timeout_errors.load(std::memory_order_relaxed) > 0) {
            return false;
        }

        // Check communication freshness
        int64_t ms_since = get_ms_since_last_communication();
        if (ms_since > 5000 && ms_since != -1) { // >5s is stale
            return false;
        }

        // Check error rate
        if (get_error_rate() > 0.01) { // >1% error rate
            return false;
        }

        return true;
    }
};

} // namespace ECU

#endif // ECU_HEALTH_METRICS_H
