/*
 * ECUHealthMetrics Implementation
 *
 * NASA Power of 10 Compliant Implementation
 */

#include "ECUHealthMetrics.h"

namespace ECU {

HealthMetricsSnapshot HealthMetrics::snapshot() const {
    HealthMetricsSnapshot snap;

    // Atomic loads with relaxed ordering (metrics are informational)
    // Using relaxed ordering is safe because these are independent counters,
    // not used for synchronization between threads

    // Buffer management
    snap.buffer_overruns = buffer_overruns.load(std::memory_order_relaxed);
    snap.packet_pool_exhaustions =
        packet_pool_exhaustions.load(std::memory_order_relaxed);

    // Performance tracking
    snap.max_loop_latency_us =
        max_loop_latency_us.load(std::memory_order_relaxed);
    snap.avg_loop_latency_us =
        avg_loop_latency_us.load(std::memory_order_relaxed);

    // Error tracking
    snap.consecutive_checksum_errors =
        consecutive_checksum_errors.load(std::memory_order_relaxed);
    snap.consecutive_timeout_errors =
        consecutive_timeout_errors.load(std::memory_order_relaxed);
    snap.total_recovery_cycles =
        total_recovery_cycles.load(std::memory_order_relaxed);

    // Legacy telemetry migration
    snap.total_ack_mismatches =
        total_ack_mismatches.load(std::memory_order_relaxed);
    snap.total_end_marker_errors =
        total_end_marker_errors.load(std::memory_order_relaxed);
    snap.total_retry_events =
        total_retry_events.load(std::memory_order_relaxed);

    // Communication statistics
    snap.total_packets_sent =
        total_packets_sent.load(std::memory_order_relaxed);
    snap.total_packets_received =
        total_packets_received.load(std::memory_order_relaxed);
    snap.total_bytes_sent = total_bytes_sent.load(std::memory_order_relaxed);
    snap.total_bytes_received =
        total_bytes_received.load(std::memory_order_relaxed);

    // Timing information (non-atomic, only written by worker thread)
    // Safe to read because time_point is trivially copyable
    snap.last_successful_communication = last_successful_communication;
    snap.thread_start_time = thread_start_time;

    return snap;
}

void HealthMetrics::reset() {
    // Reset all atomic counters to zero
    // Use relaxed ordering since this is not a synchronization operation

    // Buffer management
    buffer_overruns.store(0, std::memory_order_relaxed);
    packet_pool_exhaustions.store(0, std::memory_order_relaxed);

    // Performance tracking
    max_loop_latency_us.store(0, std::memory_order_relaxed);
    avg_loop_latency_us.store(0, std::memory_order_relaxed);

    // Error tracking
    consecutive_checksum_errors.store(0, std::memory_order_relaxed);
    consecutive_timeout_errors.store(0, std::memory_order_relaxed);
    total_recovery_cycles.store(0, std::memory_order_relaxed);

    // Legacy telemetry migration
    total_ack_mismatches.store(0, std::memory_order_relaxed);
    total_end_marker_errors.store(0, std::memory_order_relaxed);
    total_retry_events.store(0, std::memory_order_relaxed);

    // Communication statistics
    total_packets_sent.store(0, std::memory_order_relaxed);
    total_packets_received.store(0, std::memory_order_relaxed);
    total_bytes_sent.store(0, std::memory_order_relaxed);
    total_bytes_received.store(0, std::memory_order_relaxed);

    // Reset timing information
    thread_start_time = std::chrono::steady_clock::now();
    last_successful_communication = std::chrono::steady_clock::time_point{};
}

} // namespace ECU
