# spdlog

**Fast C++ logging library.**

![C++](https://img.shields.io/badge/C%2B%2B-11%2B-blue) ![License](https://img.shields.io/badge/license-MIT-green) ![Platform](https://img.shields.io/badge/platform-cross--platform-lightgrey)

spdlog is a high-performance, header-only C++ logging library designed for speed and ease of use. It provides a simple API for producing well-formatted log output with minimal overhead. Whether you're building a small utility or a large-scale production system, spdlog offers the flexibility and throughput you need without sacrificing readability.

---

## Project Overview

spdlog is a lightweight, fast logging library for C++ that focuses on performance while maintaining a clean, modern API. It is designed to be easy to integrate into any C++ project — typically as a header-only library — making it a popular choice for both hobby projects and production-grade software.

The library supports multiple logging backends (called "sinks"), including console output, file rotation, and custom destinations. It is designed with a minimal footprint and thread safety in mind, allowing concurrent logging from multiple threads without contention.

---

## Features

- 🚀 **Extremely fast** — Optimized for high-throughput logging with minimal locking overhead.
- 📦 **Header-only** — No compiled library required; just drop the headers into your project.
- 🎨 **Multiple sinks** — Console, basic file, rotating file, daily file, and more out of the box.
- 🧵 **Thread-safe** — Safe to use from multiple threads concurrently by default.
- ⚙️ **Custom formatters** — Create your own log format strings or write custom formatters.
- 🔄 **Log levels** — Built-in support for trace, debug, info, warn, error, critical, and off levels.
- 🪟 **Cross-platform** — Works on Linux, macOS, Windows, and other platforms with a C++11 or newer compiler.
- 🎯 **Simple API** — Intuitive macros and functions that feel natural to use.

---

## Requirements

- **C++ compiler** with C++11 support or newer (GCC 4.8+, Clang 3.3+, MSVC 2015+)
- **CMake** 3.1+ (if building tests or examples)
- **Operating System**: Linux, macOS, or Windows

### Optional

- [fmtlib](https://github.com/fmtlib/fmt) — spdlog can optionally use fmtlib for faster formatting (bundled version may be included).
- Git — for cloning the repository.

---

## Installation

### Option 1: Header-Only (Recommended)

Clone or download spdlog and add the `include` directory to your project's include path:

```bash
git clone https://github.com/gabime/spdlog.git
```

Then in your compiler settings, add the include path:

```bash
g++ -I path/to/spdlog/include -std=c++11 my_app.cpp -o my_app
```

### Option 2: As a Compiled Library with CMake

```bash
git clone https://github.com/gabime/spdlog.git
cd spdlog
mkdir build && cd build
cmake .. -DSPDLOG_BUILD_EXAMPLE=OFF -DSPDLOG_BUILD_TESTS=OFF
cmake --build .
cmake --install .
```

### Option 3: Using a Package Manager

spdlog is available in several package managers:

```bash
# vcpkg
vcpkg install spdlog

# Conan
conan install spdlog

# Homebrew (macOS)
brew install spdlog

# Linux (Debian/Ubuntu)
sudo apt-get install libspdlog-dev
```

---

## Quick Start

Get up and running in just a few lines:

```cpp
#include "spdlog/spdlog.h"

int main() {
    // Log to console
    spdlog::info("Welcome to spdlog!");
    spdlog::set_level(spdlog::level::debug); // Set global log level
    spdlog::debug("This is a debug message");
    spdlog::warn("This is a warning");
    
    return 0;
}
```

**Expected output:**

```
[2024-01-15 10:30:45.123] [info] Welcome to spdlog!
[2024-01-15 10:30:45.123] [debug] This is a debug message
[2024-01-15 10:30:45.123] [warn] This is a warning
```

---

## Usage

### Basic Console Logging

```cpp
#include "spdlog/spdlog.h"

int main() {
    spdlog::info("Hello, spdlog!");
    spdlog::error("Something went wrong: {}", "disk full");
    spdlog::warn("Temperature is {} degrees", 98.6);
    spdlog::debug("Retrieved {} items", 42);
    spdlog::trace("Detailed trace info");
}
```

### Logging to a File

```cpp
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

int main() {
    // Create a file logger
    auto logger = spdlog::basic_logger_mt("file_logger", "logs/app.log", true);
    logger->info("This goes to the file");
    logger->error("Error code: {}", 500);
}
```

### Rotating File Logger

```cpp
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

int main() {
    // Max 5MB per file, max 3 rotated files
    auto logger = spdlog::rotating_logger_mt("rotating_logger", "logs/rotated.log", 1048576 * 5, 3);
    
    for (int i = 0; i < 10000; ++i) {
        logger->info("Log entry {}", i);
    }
}
```

### Custom Log Format

```cpp
#include "spdlog/spdlog.h"

int main() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] %v");
    spdlog::info("Custom formatted message");
}
```

---

## Configuration

spdlog is primarily configured through its API rather than external configuration files. Key configuration options are set at runtime:

| Option | Description | Default |
|--------|-------------|---------|
| Log Level | Controls minimum severity of logged messages | `info` |
| Pattern | Defines the output format string | `[%Y-%m-%d %H:%M:%S.%e] [%l] %v` |
| Flush Policy | When log buffers are flushed to the sink | Depends on sink |

### Setting Log Level

```cpp
// Set globally
spdlog::set_level(spdlog::level::debug);

// Set per-logger
logger->set_level(spdlog::level::trace);
```

### Available Log Levels

| Level | Macro | Description |
|-------|-------|-------------|
| `trace` | `SPDLOG_TRACE(...)` | Fine-grained diagnostic information |
| `debug` | `SPDLOG_DEBUG(...)` | Debug-level messages |
| `info` | `SPDLOG_INFO(...)` | Informational messages |
| `warn` | `SPDLOG_WARN(...)` | Warning messages |
| `error` | `SPDLOG_ERROR(...)` | Error messages |
| `critical` | `SPDLOG_CRITICAL(...)` | Critical/fatal errors |
| `off` | — | Disables all logging |

---

## Architecture

spdlog follows a simple, composable architecture centered around **loggers** and **sinks**:

```
┌──────────────┐      ┌──────────────┐      ┌─────────────────┐
│   Logger     │─────▶│    Sink      │─────▶│  Destination    │
│  (front-end) │      │  (backend)   │      │  (console/file) │
└──────────────┘      └──────────────┘      └─────────────────┘
```

- **Logger** — The front-end that users interact with. Handles formatting and routing messages.
- **Sink** — The backend that receives formatted messages and writes them to a destination (console, file, syslog, etc.).
- **Sink Formatter** — Transforms log records into the final output string based on a pattern.

Key design decisions:

- **Thread safety** is achieved via per-sink mutex locking (lock per sink, not per logger).
- **Memory pool** allocation is used internally to minimize allocation overhead.
- **Asynchronous logging** is supported via `spdlog::async_logger`, which queues messages to be processed by a background thread.

---

## Project Structure

A typical spdlog repository layout:

```
spdlog/
├── include/
│   └── spdlog/
│       ├── spdlog.h              # Main include header
│       ├── common.h              # Common definitions
│       ├── details/              # Internal implementation details
│       ├── sinks/                # Various sink implementations
│       │   ├── basic_file_sink.h
│       │   ├── rotating_file_sink.h
│       │   ├── daily_file_sink.h
│       │   ├── stdout_sinks.h
│       │   └── ...
│       ├── async.h               # Async logger support
│       ├── formatter.h           # Log formatting utilities
│       └── pattern_formatter.h   # Pattern-based formatting
├── example/                      # Example programs
├── tests/                        # Unit tests
├── bench/                        # Benchmarks
├── CMakeLists.txt                # CMake build configuration
├── LICENSE                       # License file
└── README.md                     # Project README
```

---

## Development

### Building from Source

```bash
git clone https://github.com/gabime/spdlog.git
cd spdlog
mkdir build && cd build
cmake .. -DSPDLOG_BUILD_EXAMPLE=ON -DSPDLOG_BUILD_TESTS=ON
cmake --build .
```

### CMake Options

| Option | Description | Default |
|--------|-------------|---------|
| `SPDLOG_BUILD_EXAMPLE` | Build example programs | `OFF` |
| `SPDLOG_BUILD_TESTS` | Build unit tests | `OFF` |
| `SPDLOG_BUILD_BENCH` | Build benchmarks | `OFF` |
| `SPDLOG_FMT_EXTERNAL` | Use external fmt library | `OFF` |

### Running Tests

```bash
cd build
./spdlog_tests
```

---

## Contributing

We'd love your help making spdlog even better! Here's how to get started:

1. **Fork** the repository on GitHub.
2. **Create a branch** for your feature or bug fix.
3. **Make your changes** and ensure tests pass.
4. **Submit a pull request** with a clear description of your changes.

### Guidelines

- Follow the existing code style.
- Write tests for new features when possible.
- Keep commits focused and well-described.
- Open an issue first for large changes to discuss the approach.

---

## License

spdlog is licensed under the **MIT License**. See the [LICENSE](https://github.com/gabime/spdlog/blob/v1.x/LICENSE) file for details.