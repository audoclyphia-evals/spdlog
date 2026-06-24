// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.  // This header file declares the tcp_sink_config struct and tcp_sink class, which are part of spdlog's sink infrastructure for TCP-based log delivery. It includes dependencies such as base_sink for core logger functionality, tcp_client for network communication, and standard libraries for chrono formatting utilities to handle timing in reconnection strategies.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#include <spdlog/common.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/sinks/base_sink.h>
#ifdef _WIN32
#include <spdlog/details/tcp_client-windows.h>
#else
#include <spdlog/details/tcp_client.h>
#endif

#include <chrono>
#include <functional>
#include <mutex>
#include <string>

/**
 * This namespace block defines the tcp_sink_config struct and the tcp_sink class template, providing a complete implementation for TCP log sinking. It integrates with spdlog's core logger functionality by extending base_sink, and includes configuration for connection parameters, reconnection backoff using chrono utilities, and log message formatting via the base sink's formatter.
 */

SPDLOG_NAMESPACE_BEGIN
namespace sinks {

/**
 * The tcp_sink_config struct holds configuration parameters for tcp_sink, including server_host and server_port for connection, timeout_ms for socket operations, lazy_connect flag to defer connection to first log call, and reconnection settings with reconnect_delay_ms and max_reconnect_delay_ms. These settings enable controlled reconnection strategies to avoid blocking the logging thread, utilizing chrono formatting utilities for timing management.
 */
struct tcp_sink_config {
    std::string server_host;
    int server_port;
    int timeout_ms =
        0;  // The timeout for all 3 major socket operations that is connect, send, and recv
    bool lazy_connect = false;  // if true connect on first log call instead of on construction
    // Reconnect backoff: initial delay in ms, doubles each failed attempt up to max_reconnect_delay_ms
    int reconnect_delay_ms = 500;
    int max_reconnect_delay_ms = 30000;

    tcp_sink_config(std::string host, int port)
        : server_host{std::move(host)},
          server_port{port} {}
};

template <typename Mutex>

/**
 * tcp_sink is a template class inheriting from base_sink that implements a TCP log sink for sending formatted log messages to a remote server. It manages connection state, handles automatic reconnection with exponential backoff using chrono utilities to prevent thread blocking, and integrates with spdlog's logger registration and global configuration for seamless log routing over TCP.
 */
class tcp_sink : public sinks::base_sink<Mutex> {
public:
    // connect to tcp host/port or throw if failed
    // host can be hostname or ip address

    explicit tcp_sink(const std::string &host,
                      int port,
                      int timeout_ms = 0,
                      bool lazy_connect = false)
        : config_{host, port} {
        config_.timeout_ms = timeout_ms;
        config_.lazy_connect = lazy_connect;
        if (!config_.lazy_connect) {
            client_.connect(config_.server_host, config_.server_port, config_.timeout_ms);
        }
    }

    /**
     * This constructor for tcp_sink accepts a tcp_sink_config and initializes the sink. It optionally establishes an immediate connection to the remote server unless the lazy_connect flag is set, which defers connection to the first log call. This supports benchmarking tools and core infrastructure by allowing flexible initialization patterns for performance testing.
     */
    explicit tcp_sink(tcp_sink_config sink_config)
        : config_{std::move(sink_config)} {
        if (!config_.lazy_connect) {
            client_.connect(config_.server_host, config_.server_port, config_.timeout_ms);
        }
    }

    ~tcp_sink() override = default;

protected:

    /**
     * The sink_it_ method overrides the base sink's method to format the log message using spdlog's formatter, check the TCP client's connection status, and implement reconnection logic with exponential backoff. It uses chrono formatting utilities to manage timing and ensures non-blocking behavior during server unavailability, which is crucial for maintaining log throughput in core logger functionality.
     */
    void sink_it_(const details::log_msg &msg) override {
        memory_buf_t formatted;
        sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        if (!client_.is_connected()) {
            // Only attempt reconnect if enough time has elapsed since the last failed attempt.
            // This prevents blocking the logging thread on every message when the server is down.
            const auto now = std::chrono::steady_clock::now();
            if (now >= next_reconnect_time_) {
                try {
                    client_.connect(config_.server_host, config_.server_port, config_.timeout_ms);
                    // Successful reconnect — reset backoff
                    current_reconnect_delay_ms_ = config_.reconnect_delay_ms;
                } catch (...) {
                    // Reconnect failed: apply exponential backoff and drop this message
                    next_reconnect_time_ =
                        now + std::chrono::milliseconds(current_reconnect_delay_ms_);
                    current_reconnect_delay_ms_ =
                        std::min(current_reconnect_delay_ms_ * 2, config_.max_reconnect_delay_ms);
                    return;
                }
            } else {
                // Still in backoff window — drop message silently
                return;
            }
        }
        client_.send(formatted.data(), formatted.size());
    }

    void flush_() override {}
    tcp_sink_config config_;
    details::tcp_client client_;

private:
    // Reconnect backoff state (not protected by the sink's own mutex — only accessed
    // inside sink_it_() which base_sink already calls under lock for _mt variant)
    int current_reconnect_delay_ms_{config_.reconnect_delay_ms};
    std::chrono::steady_clock::time_point next_reconnect_time_{};
};

using tcp_sink_mt = tcp_sink<std::mutex>;
using tcp_sink_st = tcp_sink<details::null_mutex>;

}  // namespace sinks
SPDLOG_NAMESPACE_END
