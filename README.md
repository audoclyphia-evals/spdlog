# spdlog

Fast C++ logging library.

![Tests](https://img.shields.io/badge/tests-passing-brightgreen.svg)

spdlog is a fast, header-based C++ logging library that provides thread-safe logging infrastructure with support for multiple output destinations, pattern formatting, and asynchronous logging. The library is designed for high performance and low latency, making it suitable for a wide range of applications from simple console logging to complex multi-sink file rotation scenarios.

## Overview

spdlog provides a comprehensive logging framework built around three core concepts: loggers, sinks, and formatters. Loggers are the primary interface for emitting log messages at various severity levels. Sinks define where log output is sent — to files, the console, syslog, systemd journal, or custom destinations. Formatters control the layout and content of log messages through configurable pattern strings.

The library supports both synchronous and asynchronous logging modes. In synchronous mode, log messages are written immediately on the calling thread. In asynchronous mode, messages are queued and processed by a dedicated thread pool, minimizing the impact on application performance.

spdlog also includes a global registry that manages logger instances, supports configuration via environment variables and command-line arguments, and provides utilities for periodic flushing, backtrace capture, and mapped diagnostic context (MDC).

## Features

- **Thread-safe logging infrastructure** — Foundational components for safe concurrent log output across multiple threads
- **Multiple sink types** — File sinks (basic, rotating, daily, hourly), console sinks (stdout, stderr with color support), syslog, systemd journal, Android logcat, TCP, UDP, and more
- **Configurable pattern formatting** — Customizable log message layout with date, time, level, thread ID, source location, and user-defined flags
- **Asynchronous logging** — Non-blocking log output via a dedicated thread pool with a multi-producer, multi-consumer blocking queue
- **File rotation** — Size-based and time-based (daily, hourly) rotating file sink implementations for log file management
- **Log level management** — Configurable severity levels per logger with support for threshold filtering
- **Backtrace support** — Circular buffer for capturing recent log messages to aid in debugging
- **Mapped Diagnostic Context (MDC)** — Thread-local key-value storage for contextual log enrichment (synchronous mode only)
- **Rate limiting** — Sink that enforces a maximum message count per time window, emitting summary messages and tracking dropped counts
- **Chrono formatting utilities** — Time and date formatting with locale-aware parsing and duration formatting
- **Platform-specific support** — Windows console color sinks, POSIX system utilities, and Android system log integration
- **Sink composition** — Distribution sink for forwarding messages to multiple sub-sinks with thread-safe access, duplicate filter sink for suppressing repeated messages, and rate limiting sink for throughput control
- **Qt integration** — Sink classes for outputting to QTextEdit widgets with color support

## Requirements

- A C++17 or later compiler (tested with GCC, Clang, and MSVC)
- CMake (for building tests and examples)

## Installation

spdlog is primarily a header-based library. The core headers reside in `include/spdlog/` and can be used directly by adding the `include` directory to the compiler's include path. A small number of compiled source files in `src/` provide optional functionality and must be compiled and linked when used.

### Header-only usage (default)

Add the `include/` directory to your compiler's search path and include the main header:

```cpp
#include <spdlog/spdlog.h>
```

### Building from source

To build the compiled sources, tests, and examples, use CMake:

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

The compiled library (if using `SPDLOG_COMPILED_LIB`) provides the implementations for async logging, configuration parsing, and sink support.

## Quick Start

The fastest way to use spdlog is through the default logger, which is automatically created and configured for console output with color support.

```cpp
#include <spdlog/spdlog.h>

int main() {
    // Use the default logger (console output with colors)
    spdlog::info("Welcome to spdlog!");

    // Log messages at different severity levels
    spdlog::warn("This is a warning message");
    spdlog::error("An error occurred: code {}", 42);

    // Create a file logger
    auto file_logger = spdlog::basic_logger_mt("file_logger", "logs/basic.log");
    file_logger->info("Logging to a file");

    return 0;
}
```

Expected output (console):
```
[2024-01-15 10:30:45.123] [info] Welcome to spdlog!
[2024-01-15 10:30:45.123] [warning] This is a warning message
[2024-01-15 10:30:45.123] [error] An error occurred: code 42
```

## Usage

### Basic logging with the default logger

The `spdlog` namespace provides convenience functions for the default logger:

```cpp
#include <spdlog/spdlog.h>

void basic_example() {
    spdlog::info("Hello, {}!", "World");
    spdlog::debug("This debug message is hidden by default");
    spdlog::set_level(spdlog::level::debug);  // Enable debug output
    spdlog::debug("Now visible");
}
```

### Creating named loggers

Named loggers allow separate configuration of output destinations and log levels:

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

void named_logger_example() {
    // Create a file logger that writes to "logs/file.txt"
    auto logger = spdlog::basic_logger_mt("my_logger", "logs/file.txt");

    // Set the log level for this specific logger
    logger->set_level(spdlog::level::warn);

    logger->info("This message will not appear (level is too low)");
    logger->warn("This warning will be written to the file");
    logger->error("Error encountered: {}", 500);
}
```

### Custom pattern formatting

Control the format of log output using pattern flags:

```cpp
#include <spdlog/spdlog.h>

void pattern_example() {
    // Change the pattern of the default logger
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");

    spdlog::info("Custom pattern applied");

    // Pattern with source location
    spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e [%l] [%s:%#] %v");
    spdlog::info("Shows source file and line number");
}
```

### Rotating file logger

Automatically rotate log files when they reach a specified size:

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

void rotating_file_example() {
    // Create a rotating file logger
    // - Max file size: 5 MB
    // - Keep up to 3 rotated files
    auto logger = spdlog::rotating_logger_mt("rotating_logger", "logs/rotating.log", 5 * 1024 * 1024, 3);

    for (int i = 0; i < 10000; ++i) {
        logger->info("Log entry number {}", i);
    }
}
```

### Multiple sinks

Send log output to multiple destinations simultaneously:

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

void multi_sink_example() {
    // Create a console sink with colors
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::info);
    console_sink->set_pattern("[console] [%^%l%$] %v");

    // Create a file sink
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multi.log");
    file_sink->set_level(spdlog::level::trace);
    file_sink->set_pattern("[file] %Y-%m-%d %H:%M:%S.%e [%l] %v");

    // Create a logger with both sinks
    auto logger = std::make_shared<spdlog::logger>("multi_sink_logger",
        spdlog::sinks_init_list{console_sink, file_sink});
    logger->set_level(spdlog::level::debug);

    spdlog::register_logger(logger);
    logger->warn("This message goes to both console and file");
}
```

### Asynchronous logging

Use a thread pool for non-blocking log output:

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

void async_example() {
    // Set the thread pool size and queue size
    spdlog::init_thread_pool(8192, 1);

    // Create an async file logger
    auto async_logger = spdlog::basic_logger_mt<spdlog::async_factory>("async_logger", "logs/async.log");
    async_logger->info("This message is logged asynchronously");
    async_logger->info("Multiple messages are batched for performance");
}
```

### Rate limiting

Limit log message throughput to a configurable maximum per time window:

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/rate_limit_sink.h>

void rate_limit_example() {
    // Allow at most 100 messages per 5-second window
    auto rate_logger = spdlog::rate_limit_logger_mt(
        "rate_limited", 100, std::chrono::seconds(5));

    // Messages exceeding the limit are dropped; a summary is emitted
    // when the window resets.
    for (int i = 0; i < 200; ++i) {
        rate_logger->info("Message {}", i);
    }
}
```

### Duplicate filter sink

Suppress repeated log messages within a configurable time window, emitting a summary when duplicates resume:

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/dup_filter_sink.h>

void dup_filter_example() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

    // Suppress duplicate messages within a 5-second window.
    // The summary message is emitted at the highest severity seen
    // during the skip window.
    auto dup_sink = std::make_shared<spdlog::sinks::dup_filter_sink_mt>(
        std::chrono::seconds(5),
        spdlog::sinks_init_list{console_sink});

    auto logger = std::make_shared<spdlog::logger>("dup_filtered", dup_sink);

    for (int i = 0; i < 100; ++i) {
        logger->info("Repeated message");  // Only logged once per 5s window
    }
    // After the window expires and a new unique message arrives, a summary
    // like "Skipped 99 duplicate messages.." is emitted.
}
```

The `dup_filter_sink` inherits from `dist_sink`, so it composes with any number of downstream sinks. The skip duration determines how long a message pattern must remain unchanged before duplicates are suppressed.

### Distribution sink

The `dist_sink` forwards log messages to multiple sub-sinks, enabling fan-out to different outputs. It serves as the base class for composite sinks like `dup_filter_sink` and `rate_limit_sink`:

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dist_sink.h>

void dist_sink_example() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/dist.log");

    auto dist = std::make_shared<spdlog::sinks::dist_sink_mt>();
    dist->add_sink(console_sink);
    dist->add_sink(file_sink);

    auto logger = std::make_shared<spdlog::logger>("distributed", dist);
    logger->info("Sent to both console and file");

    // Sinks can be added or removed at runtime (thread-safe)
    dist->remove_sink(file_sink);
    dist->add_sink(file_sink);
}
```

The `sinks()` method returns a thread-safe copy of the current sink list. The `dist_sink` propagates pattern and formatter changes to all sub-sinks automatically.

### Backtrace support

Capture recent log messages for debugging intermittent issues:

```cpp
#include <spdlog/spdlog.h>

void backtrace_example() {
    spdlog::enable_backtrace(10);  // Keep last 10 messages in the ring buffer

    for (int i = 0; i < 5; ++i) {
        spdlog::debug("Backtrace entry {}", i);
    }

    // Dump the backtrace at a higher level
    spdlog::dump_backtrace();
}
```

### TCP sink with reconnection backoff

The TCP sink automatically reconnects when the connection drops, using exponential backoff to avoid blocking the logging thread when the server is unavailable:

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/tcp_sink.h>

void tcp_example() {
    spdlog::sinks::tcp_sink_config config("localhost", 9876);
    config.lazy_connect = true;          // connect on first log call
    config.timeout_ms = 3000;            // 3-second socket timeout
    config.reconnect_delay_ms = 500;     // initial reconnect delay
    config.max_reconnect_delay_ms = 30000; // cap at 30 seconds

    auto tcp_logger = spdlog::create<spdlog::sinks::tcp_sink_mt>("tcp_logger", config);
    tcp_logger->info("Message sent over TCP");
}
```

## 📚 Additional Documentation

For more detailed information, see the following documentation:

- [spdlog Architecture Overview](ARCHITECTURE.md) - Explains the overall system design, component relationships, and key modules to aid developer understanding and contribution, especially useful given the library's modular structure.
- [Contributing to spdlog](CONTRIBUTING.md) - Provides guidelines for development, testing, and contribution workflows to encourage community involvement and maintain code quality.
- [API Documentation](api_documentation.yaml) - Generated API reference file