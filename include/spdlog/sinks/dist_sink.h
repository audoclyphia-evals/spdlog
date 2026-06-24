// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include "base_sink.h"
#include <spdlog/details/log_msg.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/pattern_formatter.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

/**
 * Namespace block encapsulating the dist_sink class implementation within spdlog's sink infrastructure. It defines the template class for log message distribution and provides type aliases for thread-safe (dist_sink_mt) and non-thread-safe (dist_sink_st) configurations, integrating with the core logger functionality for multi-sink routing.
 */

SPDLOG_NAMESPACE_BEGIN
namespace sinks {

template <typename Mutex>

/**
 * dist_sink is a template class inheriting from base_sink that enables distribution of log messages to multiple sub-sinks. It includes methods for adding, removing, and setting sinks dynamically, overrides base methods for pattern and formatter propagation, and ensures synchronized access via the Mutex parameter, supporting core logger registration and global configuration in spdlog's architecture.
 */
class dist_sink : public base_sink<Mutex> {
public:
    dist_sink() = default;
    explicit dist_sink(std::vector<std::shared_ptr<sink>> sinks)
        : sinks_(std::move(sinks)) {}

    dist_sink(const dist_sink &) = delete;
    dist_sink &operator=(const dist_sink &) = delete;

    void add_sink(std::shared_ptr<sink> sub_sink) {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        sinks_.push_back(sub_sink);
    }

    void remove_sink(std::shared_ptr<sink> sub_sink) {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        sinks_.erase(std::remove(sinks_.begin(), sinks_.end(), sub_sink), sinks_.end());
    }

    void set_sinks(std::vector<std::shared_ptr<sink>> sinks) {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        sinks_ = std::move(sinks);
    }

    // Returns a *copy* of the current sinks vector to avoid exposing the
    // internal vector to unsynchronised external mutation (data race on _mt).
    std::vector<std::shared_ptr<sink>> sinks() const {
        std::lock_guard<Mutex> lock(base_sink<Mutex>::mutex_);
        return sinks_;
    }

protected:
    void sink_it_(const details::log_msg &msg) override {
        for (auto &sub_sink : sinks_) {
            if (sub_sink->should_log(msg.level)) {
                sub_sink->log(msg);
            }
        }
    }

    void flush_() override {
        for (auto &sub_sink : sinks_) {
            sub_sink->flush();
        }
    }

    void set_pattern_(const std::string &pattern) override {
        set_formatter_(details::make_unique<pattern_formatter>(pattern));
    }

    void set_formatter_(std::unique_ptr<formatter> sink_formatter) override {
        base_sink<Mutex>::formatter_ = std::move(sink_formatter);
        for (auto &sub_sink : sinks_) {
            sub_sink->set_formatter(base_sink<Mutex>::formatter_->clone());
        }
    }
    std::vector<std::shared_ptr<sink>> sinks_;
};

using dist_sink_mt = dist_sink<std::mutex>;
using dist_sink_st = dist_sink<details::null_mutex>;

}  // namespace sinks
SPDLOG_NAMESPACE_END
