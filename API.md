# spdlog API Overview

Fast C++ logging library.

![Tests](https://img.shields.io/badge/tests-passing-brightgreen.svg)

spdlog is a header-only C++ logging library designed for high performance and ease of use. It provides thread-safe logging with configurable output destinations (sinks), customizable formatting, and support for both synchronous and asynchronous logging modes. The library is suitable for applications requiring fast, structured logging with minimal overhead.

## Usage

### Basic Logging

The simplest way to start logging is through the default logger provided by the library.

```cpp
#include "spdlog/spdlog.h"

int main() {
    // Set global log level
    spdlog::set_level(spdlog::level::info);

    // Basic logging macros
    spdlog::info("Welcome to spdlog!");
    spdlog::warn("This is a warning message");
    spdlog::error("An error occurred: {}", 42);

    // Formatting with arguments
    spdlog::info("User {} logged in from {}", "admin", "192.168.1.1");

    return 0;
}
```

### Creating a File Logger

For persistent logging to a file, create a logger with a file sink.

```cpp
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

int main() {
    // Create a basic file logger
    auto file_logger = spdlog::basic_logger_mt("file_logger", "logs/app.log");
    file_logger->set_level(spdlog::level::trace);

    file_logger->trace("Detailed trace message");
    file_logger->info("Application started");
    file_logger->error("Failed to connect to database");

    return 0;
}
```

### Logging with Source Location

spdlog supports capturing source file, line, and function name for each log message.

```cpp
#include "spdlog/spdlog.h"

int main() {
    // Enable source location in log messages
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v");

    spdlog::info("This message includes source location");
    spdlog::warn("Warning with file and line number");

    return 0;
}
```

### Using Multiple Sinks

A logger can write to multiple destinations simultaneously.

```cpp
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

int main() {
    // Create sinks
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multi.log");

    // Create logger with multiple sinks
    spdlog::logger multi_logger("multi", {console_sink, file_sink});
    multi_logger.set_level(spdlog::level::debug);

    multi_logger.debug("This message goes to both console and file");
    multi_logger.info("Multi-sink logging is working");

    return 0;
}
```

## Configuration

### Environment Variables

spdlog supports configuration through environment variables for quick setup.

| Variable | Description | Default | Required |
|----------|-------------|---------|----------|
| `SPDLOG_LEVEL` | Global log level (trace, debug, info, warn, err, critical, off) | `info` | No |

The `SPDLOG_LEVEL` environment variable can be used to set the global log level at application startup without modifying code. For example, setting `SPDLOG_LEVEL=debug` enables debug-level logging across all loggers.

### Log Level Configuration

Log levels control which messages are output. The available levels are:

- `trace` (0) - Most detailed, for fine-grained debugging
- `debug` (1) - Debugging information
- `info` (2) - Informational messages (default)
- `warn` (3) - Warning conditions
- `err` (4) - Error conditions
- `critical` (5) - Critical/fatal errors
- `off` (6) - No logging

Levels can be set globally or per-logger:

```cpp
// Global level
spdlog::set_level(spdlog::level::debug);

// Per-logger level
auto logger = spdlog::basic_logger_mt("my_logger", "logs/app.log");
logger->set_level(spdlog::level::warn);
```

### Pattern Formatting

The log message format is configurable through pattern strings. The default pattern is `[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v`.

Common pattern flags include:

| Flag | Description |
|------|-------------|
| `%v` | The actual log message text |
| `%l` | Log level name (e.g., INFO, WARN) |
| `%^` | Start color range |
| `%$` | End color range |
| `%Y` | Year (4 digits) |
| `%m` | Month (01-12) |
| `%d` | Day (01-31) |
| `%H` | Hours (00-23) |
| `%M` | Minutes (00-59) |
| `%S` | Seconds (00-59) |
| `%e` | Milliseconds (000-999) |
| `%s` | Source file name |
| `%#` | Source line number |
| `%t` | Thread ID |
| `%P` | Process ID |
| `%n` | Logger name |

Example pattern configuration:

```cpp
spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$] %v");
```

For detailed information on custom formatting, see the [Custom Formatting in spdlog](CUSTOM_FORMATTING.md) guide.