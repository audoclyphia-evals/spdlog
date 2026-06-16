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
- **Mapped Diagnostic Context (MDC)** — Thread-local key-value storage for contextual log enrichment
- **Chrono formatting utilities** — Time and date formatting with locale-aware parsing and duration formatting
- **Platform-specific support** — Windows console color sinks, POSIX system utilities, and Android system log integration
- **Sink composition** — Distribution sink for forwarding messages to multiple sub-sinks, and duplicate filter sink for suppressing repeated messages
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

## Additional Documentation

For more detailed information, see the following documentation:

- [spdlog Architecture](ARCHITECTURE.md) — Provides a high-level overview of the library's architecture, component relationships, and design decisions, essential for new contributors and advanced users.
- [Contributing to spdlog](CONTRIBUTING.md) — Guides contributors on development environment setup, test execution, and coding conventions.
- [spdlog Usage Guide](USAGE.md) — Provides comprehensive usage examples and configuration options for common logging scenarios.
- [spdlog Sinks Reference](SINKS.md) — Documents all available sink types, their configuration parameters, and usage examples.
- [spdlog API Overview](API.md) — Provides a high-level conceptual guide to the spdlog API, including logger creation, level management, pattern formatting, and global configuration patterns.
- [Custom Formatting in spdlog](CUSTOM_FORMATTING.md) — Explains how to create custom formatters, use pattern flags, and extend formatting for user-defined types.

---

The following section provides a detailed API reference for the core components of spdlog, including endpoints for logger retrieval, logging, formatting, and sink management.

## API Reference

### Registry

#### Retrieve a logger by name

Gets a logger instance from the central registry using its unique name.

- **Path**: `logger with name`
- **Method**: GET
- **Operation ID**: `getLoggerByName`
- **Tags**: Registry

**Parameters**:
- `name` (path, required): Name of the logger to retrieve.

**Responses**:
- `200`: Logger found and returned.
- `404`: Logger not found.
- `500`: Internal server error.

#### Retrieve raw pointer to default logger

Returns the raw pointer to the default logger instance managed by the registry.

- **Path**: `registry.get_default_raw`
- **Method**: GET
- **Operation ID**: `getDefaultRaw`
- **Tags**: Registry

**Responses**:
- `200`: Success.
- `500`: Internal server error.

### Formatting

#### Dynamic formatting argument store

Provides a dynamic list of formatting arguments with storage that can be converted into `basic_format_args` for type-erased formatting functions such as `fmt::vformat`.

- **Path**: `abc`
- **Method**: GET
- **Operation ID**: `getDynamicFormatArgStore`
- **Tags**: Formatting

**Responses**:
- `200`: Success.
- `400`: Bad request.
- `401`: Unauthorized.
- `500`: Internal server error.

#### Append reference-wrapped argument to storage

Appends a reference-wrapped argument to the storage, ensuring type safety.

- **Path**: `push_back`
- **Method**: GET
- **Operation ID**: `appendReferenceWrappedArgument`
- **Tags**: Storage

**Responses**:
- `200`: Success.
- `400`: Bad request.
- `500`: Internal server error.

#### Write formatted time data to buffer

Writes a formatted time structure to a buffer, using the `time_put` facet of the provided locale for locale-specific formatting.

- **Path**: ` `
- **Method**: PUT
- **Operation ID**: `writeFormattedTimeToBuffer`
- **Tags**: TimeFormatting

**Parameters** (query):
- `format` (required): Format character for time formatting.
- `modifier` (required): Modifier character for time formatting.

**Request body** (application/json):
- `time` (object, required): Time structure (`std::tm`) with date/time fields.
- `locale` (object, required): Locale object for formatting, convertible to `std::locale`.

**Responses**:
- `200`: Time data formatted and written to buffer successfully.
- `400`: Bad request due to invalid format or modifier.
- `401`: Unauthorized.
- `500`: Internal server error during formatting.

#### Format arguments into a buffer

Formats arguments into a buffer using a format string.

- **Path**: `vformat_to`
- **Method**: GET
- **Operation ID**: `vformatTo`
- **Tags**: Formatting

**Parameters** (query):
- `buf` (required): Output buffer for formatted result.
- `fmt` (required): Format string.
- `args` (required): Format arguments (array of strings).
- `loc` (optional): Locale reference.

**Responses**:
- `200`: Success.
- `400`: Bad request.
- `500`: Internal server error.

#### Retrieve a formatted printf argument by ID

Returns the format argument at the specified index from the printf context.

- **Path**: `FMT_BEGIN_EXPORT`
- **Method**: GET
- **Operation ID**: `getPrintfArg`
- **Tags**: PrintfContext

**Parameters**:
- `id` (path, required): Argument index to retrieve.

**Responses**:
- `200`: Successfully retrieved argument.
- `400`: Invalid argument ID.
- `500`: Internal server error.

#### Retrieve a formatted argument by ID

Returns the formatted argument associated with the given ID from the printf context.

- **Path**: `arg`
- **Method**: GET
- **Operation ID**: `getFormattedArgumentById`
- **Tags**: Formatting

**Parameters**:
- `id` (path, required): Identifier of the argument to retrieve.

**Responses**:
- `200`: Retrieved formatted argument successfully.
- `400`: Invalid ID.
- `404`: Argument not found.
- `500`: Internal server error.

#### Format reference_wrapper types

Formats a given `std::reference_wrapper` by unwrapping it and using the formatter for the underlying type.

- **Path**: `format`
- **Method**: GET
- **Operation ID**: `formatReferenceWrapper`
- **Tags**: Formatters

**Parameters** (query):
- `ref` (required): `std::reference_wrapper<T>` instance.
- `ctx` (required): FormatContext providing output iterator.

**Responses**:
- `200`: Formatted output iterator.
- `400`: Invalid reference wrapper or context.
- `500`: Internal formatting error.

### Standard Library

#### Namespace containing type traits and utilities

Namespace containing type traits and utilities for formatting standard library types such as filesystem paths, variants, and error codes.

- **Path**: `std::`
- **Method**: GET
- **Operation ID**: `getStdNamespace`
- **Tags**: Standard Library, Formatting

**Responses**:
- `200`: Successfully retrieved namespace information.
- `404`: Namespace not found.
- `500`: Internal server error.

#### Get pointer value from unique_ptr

Retrieves and formats the raw pointer value from a `unique_ptr` for display or logging purposes.

- **Path**: `ptr`
- **Method**: GET
- **Operation ID**: `getPointerValue`
- **Tags**: Formatting

**Parameters** (query):
- `pointer` (required): The pointer value to format.

**Responses**:
- `200`: Pointer value formatted successfully.
- `400`: Invalid pointer value.
- `500`: Internal server error.

### Demangling

#### Write demangled C++ type names

Returns the demangled and normalized name of a C++ type, applying platform-specific transformations for libc++ and MSVC ABIs.

- **Path**: `write_demangled_name`
- **Method**: GET
- **Operation ID**: `writeDemangledName`
- **Tags**: Demangling

**Parameters** (query):
- `type_name` (required): The mangled C++ type name to demangle.

**Responses**:
- `200`: Demangled type name successfully returned.
- `400`: Invalid or missing `type_name` parameter.
- `500`: Internal demangling failure.

### Logging

#### Log a formatted message

Public endpoint to format and log a message using the logger.

- **Path**: `log`
- **Method**: GET
- **Operation ID**: `logMessage`
- **Tags**: Logging

**Parameters** (query):
- `level` (required): Log level (trace, debug, info, warn, error, critical).
- `message` (required): Format string message.
- `file` (optional): Source file for logging.
- `line` (optional): Source line number.

**Responses**:
- `200`: Message logged successfully.
- `400`: Invalid parameters.
- `401`: Unauthorized.
- `500`: Internal server error.

#### Sink log message to Kafka topic

Produces a log message to the configured Kafka topic via the sink implementation.

- **Path**: `sink_it`
- **Method**: GET
- **Operation ID**: `sinkLogMessage`
- **Tags**: Logging, Kafka

**Parameters** (query):
- `message` (required): The log message payload to be produced.

**Responses**:
- `200`: Message successfully sunk to Kafka.
- `400`: Invalid message format or missing parameter.
- `401`: Unauthorized.
- `500`: Internal server error during message production.

### Logger

#### Logger class definition

Provides the complete definition of the Logger class including constructors, logging methods, and sink management.

- **Path**: `logger`
- **Method**: GET
- **Operation ID**: `getLoggerDefinition`
- **Tags**: Logger

**Responses**:
- `200`: Logger class definition.

### Configuration (Kafka)

#### Retrieve current Kafka bootstrap servers configuration

Returns the list of Kafka bootstrap servers currently configured for the sink.

- **Path**: `bootstrap.servers`
- **Method**: GET
- **Operation ID**: `getBootstrapServers`
- **Tags**: Kafka, Configuration

**Responses**:
- `200`: Successfully retrieved bootstrap servers.
- `400`: Bad request.
- `500`: Internal server error.

### Formatting (Localized)

#### Write localized number formatting to output

Formats a number according to locale specifications and writes the result.

- **Path**: `write_loc`
- **Method**: PUT
- **Operation ID**: `writeLoc`
- **Tags**: Formatting

**Request body** (application/json):
- `value` (number, required): The numeric value to format.
- `specs` (object, required): Formatting specifications (precision, grouping, etc.).
- `loc` (string, required): Locale identifier (e.g., 'en_US').

**Responses**:
- `200`: Formatting succeeded.
- `400`: Invalid input parameters.
- `401`: Unauthorized.
- `500`: Internal server error.

### Tags

- Configuration
- Demangling
- Formatters
- Formatting
- Kafka
- Logger
- Logging
- PrintfContext
- Registry
- Standard Library
- Storage
- TimeFormatting

**Note**: The endpoint `std.shared_ptr_logger_get` (retrieve a logger by name) has been removed as it duplicates the functionality of the Registry endpoint `logger with name`.