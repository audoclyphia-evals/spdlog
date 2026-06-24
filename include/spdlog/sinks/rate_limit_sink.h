// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.  // Header file that declares the rate_limit_sink class template and associated factory functions for creating rate-limited loggers. It includes dependencies for chrono formatting utilities to handle time windows and provides a rate-limiting mechanism that controls the flow of log messages to downstream sinks.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include "dist_sink.h"
#include <spdlog/details/log_msg.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/synchronous_factory.h>

#include <chrono>
#include <cstdio>
#include <mutex>
#include <string>

/**
 * Namespace block within the spdlog::sinks namespace that fully implements the rate_limit_sink class template, including constructors, the rate-limiting core logic in sink_it_, and methods for managing summary levels and drop counters. This encapsulates the rate-limiting functionality for log message processing.
 */

SPDLOG_NAMESPACE_BEGIN
namespace sinks {

template <typename Mutex>

/**
 * Template class that implements a rate-limiting sink, inheriting from dist_sink. It enforces a maximum of max_count messages per window_duration using chrono-based time tracking. Class members manage per-window state (counts, window start) and lifetime drop counters, with a configurable summary_level for emitting drop summaries.
 */
class rate_limit_sink : public dist_sink<Mutex> {
public:
    // Allow at most `max_count` messages during each `window_duration` interval.
    template <class Rep, class Period>
    rate_limit_sink(size_t max_count, std::chrono::duration<Rep, Period> window_duration)
        : max_count_(max_count),
          window_duration_(
              std::chrono::duration_cast<std::chrono::microseconds>(window_duration)) {}

    // Constructor that also pre-populates child sinks.
    template <class Rep, class Period>
    rate_limit_sink(size_t max_count,
                    std::chrono::duration<Rep, Period> window_duration,
                    std::vector<std::shared_ptr<sink>> sinks)
        : dist_sink<Mutex>(std::move(sinks)),
          max_count_(max_count),
          window_duration_(
              std::chrono::duration_cast<std::chrono::microseconds>(window_duration)) {}

    /**
     * Sets the log level (summary_level_) at which the sink emits a summary message indicating the number of dropped messages in the previous window. Uses mutex locking to ensure thread-safe updates to the configuration in multi-threaded contexts.
     */
    void set_summary_level(level::level_enum lvl) {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        summary_level_ = lvl;
    }

    /**
     * Returns the current summary log level (summary_level_), which determines the verbosity of the drop summary messages. Access is protected by a mutex to maintain consistency in concurrent logging scenarios.
     */
    level::level_enum summary_level() const {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        return summary_level_;
    }

    /**
     * Returns the total number of messages dropped by the sink since its creation or last reset (total_dropped_). This provides a cumulative metric for rate-limiting effectiveness, with thread-safe access via mutex.
     */
    size_t drop_counter() const {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        return total_dropped_;
    }

    /**
     * Resets the total_dropped_ counter to zero, allowing users to monitor drop counts over specific intervals. Ensures atomic operation through mutex synchronization to avoid race conditions.
     */
    void reset_drop_counter() {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        total_dropped_ = 0;
    }

protected:

    /**
     * Override of the sink_it_ method that implements the rate-limiting logic. It checks window expiration, emits a summary for dropped messages if the window resets, and either forwards the message to child sinks (if within quota) or increments drop counters. Uses chrono utilities for time comparison and manages window state.
     */
    void sink_it_(const details::log_msg &msg) override {
        const auto now = msg.time;

        // ── Window management ──────────────────────────────────────────────
        // Check whether the current window has expired.
        if (now - window_start_ >= window_duration_) {
            // Emit a summary of messages dropped in the previous window.
            if (dropped_in_window_ > 0) {
                char buf[80];
                auto n = ::snprintf(buf, sizeof(buf),
                                    "Rate limit: dropped %zu messages in last window",
                                    dropped_in_window_);
                if (n > 0 && static_cast<size_t>(n) < sizeof(buf)) {
                    details::log_msg summary_msg{msg.source, msg.logger_name, summary_level_,
                                                 string_view_t{buf, static_cast<size_t>(n)}};
                    dist_sink<Mutex>::sink_it_(summary_msg);
                }
            }
            // Open a new window starting from the current message's timestamp.
            window_start_ = now;
            count_in_window_ = 0;
            dropped_in_window_ = 0;
        }

        // ── Rate check ────────────────────────────────────────────────────
        if (count_in_window_ < max_count_) {
            dist_sink<Mutex>::sink_it_(msg);
            ++count_in_window_;
        } else {
            ++dropped_in_window_;
            ++total_dropped_;
        }
    }

private:
    // Configuration (immutable after construction)
    size_t max_count_;
    std::chrono::microseconds window_duration_;

    // Per-window state (all accessed only inside sink_it_(), which base_sink
    // already protects under mutex_ for the _mt variant)
    size_t count_in_window_{0};
    size_t dropped_in_window_{0};
    log_clock::time_point window_start_{};

    // Lifetime counters
    size_t total_dropped_{0};

    // Level at which the "dropped N messages" summary is emitted
    level::level_enum summary_level_{level::warn};
};

using rate_limit_sink_mt = rate_limit_sink<std::mutex>;
using rate_limit_sink_st = rate_limit_sink<details::null_mutex>;

}  // namespace sinks

//
// Factory functions — create a synchronous logger backed by a rate_limit_sink.
//
// `max_count`       — maximum messages allowed per window
// `window_duration` — length of each rolling window (any std::chrono duration)
//
template <typename Factory = synchronous_factory>

/**
 * Factory function that creates a multi-threaded logger (logger with rate_limit_sink_mt sink) using the synchronous_factory. It configures rate limiting with max_count and window_duration, suitable for concurrent logging environments.
 */
inline std::shared_ptr<logger> rate_limit_logger_mt(const std::string &logger_name,
                                                    size_t max_count,
                                                    std::chrono::microseconds window_duration) {
    return Factory::template create<sinks::rate_limit_sink_mt>(logger_name, max_count,
                                                               window_duration);
}

template <typename Factory = synchronous_factory>

/**
 * Factory function that creates a single-threaded logger (logger with rate_limit_sink_st sink) using the synchronous_factory. Optimized for single-threaded performance with a null_mutex, applying the same rate-limiting parameters without thread-safety overhead.
 */
inline std::shared_ptr<logger> rate_limit_logger_st(const std::string &logger_name,
                                                    size_t max_count,
                                                    std::chrono::microseconds window_duration) {
    return Factory::template create<sinks::rate_limit_sink_st>(logger_name, max_count,
                                                               window_duration);
}

SPDLOG_NAMESPACE_END
