// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
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

// Rate-limiting sink.
//
// Passes at most `max_count` messages through to the child sinks within each
// rolling time window of `window_duration`.  Once the per-window quota is
// exhausted, subsequent messages are silently counted as dropped.  When the
// window resets (i.e. when the next message arrives after the window has
// expired), a summary line is emitted first:
//
//     [warn]  Rate limit: dropped 42 messages in last window
//
// The summary is emitted at `warn` level by default so it is visible even when
// child sinks suppress lower levels.  You can customise this via
// `set_summary_level()`.
//
// Unlike dup_filter_sink (which only suppresses *identical* messages),
// rate_limit_sink suppresses *any* messages once the quota is reached,
// regardless of their content.  This makes it suitable for hot code paths that
// can produce a large volume of distinct messages.
//
// Example:
//
//     #include <spdlog/sinks/rate_limit_sink.h>
//
//     // Allow at most 10 messages per second; excess messages are dropped.
//     auto rl = std::make_shared<spdlog::sinks::rate_limit_sink_mt>(
//         10, std::chrono::seconds(1));
//     rl->add_sink(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
//
//     spdlog::logger logger("my_logger", rl);
//     for (int i = 0; i < 100; ++i) {
//         logger.info("message {}", i);   // only the first 10 pass per second
//     }

SPDLOG_NAMESPACE_BEGIN
namespace sinks {

template <typename Mutex>
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

    // Change the log level used for the "Rate limit: dropped N messages" summary line.
    void set_summary_level(level::level_enum lvl) {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        summary_level_ = lvl;
    }

    level::level_enum summary_level() const {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        return summary_level_;
    }

    // Returns the total number of messages dropped since construction (or the
    // last call to reset_drop_counter()).
    size_t drop_counter() const {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        return total_dropped_;
    }

    void reset_drop_counter() {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        total_dropped_ = 0;
    }

protected:
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
inline std::shared_ptr<logger> rate_limit_logger_mt(const std::string &logger_name,
                                                    size_t max_count,
                                                    std::chrono::microseconds window_duration) {
    return Factory::template create<sinks::rate_limit_sink_mt>(logger_name, max_count,
                                                               window_duration);
}

template <typename Factory = synchronous_factory>
inline std::shared_ptr<logger> rate_limit_logger_st(const std::string &logger_name,
                                                    size_t max_count,
                                                    std::chrono::microseconds window_duration) {
    return Factory::template create<sinks::rate_limit_sink_st>(logger_name, max_count,
                                                               window_duration);
}

SPDLOG_NAMESPACE_END
