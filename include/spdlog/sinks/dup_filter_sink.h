// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include "dist_sink.h"
#include <spdlog/details/log_msg.h>
#include <spdlog/details/null_mutex.h>

#include <chrono>
#include <cstdio>
#include <mutex>
#include <string>

/**
 * This namespace encapsulates the implementation of dup_filter_sink within spdlog's sink infrastructure, providing a template class for filtering duplicate log messages and type aliases for thread-safe (dup_filter_sink_mt) and single-threaded (dup_filter_sink_st) variants.
 */

SPDLOG_NAMESPACE_BEGIN
namespace sinks {
template <typename Mutex>

/**
 * dup_filter_sink is a template class inheriting from dist_sink that filters duplicate log messages using chrono-based time durations. It includes constructors accepting a max_skip_duration parameter for deduplication windows, and maintains protected members like max_skip_duration_, last_msg_time_, last_msg_payload_, skip_counter_, and skipped_msg_log_level_ to track and manage duplicate log entries.
 */
class dup_filter_sink : public dist_sink<Mutex> {
public:
    template <class Rep, class Period>
    explicit dup_filter_sink(std::chrono::duration<Rep, Period> max_skip_duration)
        : max_skip_duration_{max_skip_duration} {}

    template <class Rep, class Period>
    explicit dup_filter_sink(std::chrono::duration<Rep, Period> max_skip_duration, std::vector<std::shared_ptr<sink>> sinks)
        : max_skip_duration_{max_skip_duration}
        , dist_sink<Mutex>(std::move(sinks)) {}

protected:
    std::chrono::microseconds max_skip_duration_;
    log_clock::time_point last_msg_time_;
    std::string last_msg_payload_;
    size_t skip_counter_ = 0;
    // Track the *highest* level seen during a skip window so the summary
    // message is never filtered by a sink's own level threshold.
    level::level_enum skipped_msg_log_level_ = level::level_enum::off;

    /**
     * The sink_it_ method overrides the base class to filter duplicate log messages within a time window. It uses filter_ to decide if a message should be skipped, increments skip_counter_ for duplicates, tracks the highest log level seen during skips, and logs a summary message with the count and highest level when a new unique message arrives, then resets state and passes the message to downstream sinks.
     */
    void sink_it_(const details::log_msg &msg) override {
        bool filtered = filter_(msg);
        if (!filtered) {
            skip_counter_ += 1;
            // Keep the highest severity seen so the "Skipped N" summary
            // is emitted at that level rather than the first duplicate's level.
            if (msg.level > skipped_msg_log_level_) {
                skipped_msg_log_level_ = msg.level;
            }
            return;
        }

        // log the "skipped.." message
        if (skip_counter_ > 0) {
            char buf[64];
            auto msg_size = ::snprintf(buf, sizeof(buf), "Skipped %u duplicate messages..",
                                       static_cast<unsigned>(skip_counter_));
            if (msg_size > 0 && static_cast<size_t>(msg_size) < sizeof(buf)) {
                details::log_msg skipped_msg{msg.source, msg.logger_name, skipped_msg_log_level_,
                                             string_view_t{buf, static_cast<size_t>(msg_size)}};
                dist_sink<Mutex>::sink_it_(skipped_msg);
            }
        }

        // log current message
        dist_sink<Mutex>::sink_it_(msg);
        last_msg_time_ = msg.time;
        skip_counter_ = 0;
        // Reset to off so the next skip window starts fresh, then seed it
        // immediately with the current message's level so that if the very
        // next message is a duplicate we already have a valid level recorded.
        skipped_msg_log_level_ = level::level_enum::off;
        last_msg_payload_.assign(msg.payload.data(), msg.payload.data() + msg.payload.size());
    }

    // return whether the log msg should be displayed (true) or skipped (false)
    bool filter_(const details::log_msg &msg) const {
        const auto filter_duration = msg.time - last_msg_time_;
        return (filter_duration > max_skip_duration_) || (msg.payload != last_msg_payload_);
    }
};

using dup_filter_sink_mt = dup_filter_sink<std::mutex>;
using dup_filter_sink_st = dup_filter_sink<details::null_mutex>;

}  // namespace sinks
SPDLOG_NAMESPACE_END
