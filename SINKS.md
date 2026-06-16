# spdlog Sinks Reference

![Tests](https://img.shields.io/badge/tests-passing-brightgreen.svg)

Reference guide for the spdlog sink system, detailing built-in sink types, their configuration, and usage patterns for directing log output to files, consoles, network endpoints, and system logs.

## Usage

All logging in spdlog is directed through **sinks** — objects that implement the `sink` interface and determine the destination of each log message. A logger can hold one or more sinks, and the library provides a variety of built-in sink implementations.

### Basic Sink Setup

The following example creates a logger with a single console sink (colored output to stdout):

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

int main()
{
    // Create a console logger with a color sink (thread-safe variant)
    auto console = spdlog::stdout_color_mt("console");
    console->info("Application started");
    console->warn("Low disk space");
    console->error("Failed to open file");

    // The default logger is also configured with a color sink
    spdlog::info("Default logger info message");
    return 0;
}
```

### File Sinks

File sinks write log output to disk. The library provides several file-based sink variants for different rotation strategies:

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>

int main()
{
    // Basic file sink — single log file, no rotation
    auto file_logger = spdlog::basic_logger_mt("file_logger", "logs/app.log");
    file_logger->info("Logging to a single file");

    // Rotating file sink — rotates by size, keeps N backup files
    auto rotating_logger = spdlog::rotating_logger_mt(
        "rotating_logger", "logs/rotating.log", 1024 * 1024, 5);
    rotating_logger->info("Log entry");

    // Daily file sink — rotates at midnight (local time)
    auto daily_logger = spdlog::daily_logger_mt(
        "daily_logger", "logs/daily.log", 23, 30);
    daily_logger->info("New day, new log file");

    return 0;
}
```

### Multi-Sink Logger

A single logger can route messages to multiple sinks simultaneously. This is useful for writing to a file and the console at the same time:

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <vector>
#include <memory>

int main()
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/multi.log");

    // Create a logger with multiple sinks
    std::vector<spdlog::sink_ptr> sinks = {console_sink, file_sink};
    auto multi_logger = std::make_shared<spdlog::logger>("multi", sinks.begin(), sinks.end());

    spdlog::register_logger(multi_logger);
    multi_logger->info("This message appears on the console and in the file");

    return 0;
}
```

### Sink Configuration with Patterns and Levels

Each sink can have its own format pattern and log level threshold:

```cpp
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

int main()
{
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::warn);
    console_sink->set_pattern("[console] [%^%l%$] %v");

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/filtered.log");
    file_sink->set_level(spdlog::level::info);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");

    auto logger = std::make_shared<spdlog::logger>("filtered", sinks);
    logger->info("This goes to the file");
    logger->warn("This goes to both sinks");
    logger->error("This also goes to both");

    return 0;
}
```

## Configuration

### Sink Type Selection

The following built-in sink types are provided, organized by category. Each sink is available in a thread-safe (`_mt`) and single-threaded (`_st`) variant where applicable.

| Sink Header | Sink Class | Output Destination |
|---|---|---|
| `stdout_sinks.h` | `stdout_sink` / `stderr_sink` | Standard output / error |
| `stdout_color_sinks.h` | `stdout_color_sink` / `stderr_color_sink` | Colored stdout / stderr |
| `basic_file_sink.h` | `basic_file_sink` | Single log file |
| `rotating_file_sink.h` | `rotating_file_sink` | Size-based rotating files |
| `daily_file_sink.h` | `daily_file_sink` | Daily rotating files |
| `hourly_file_sink.h` | `hourly_file_sink` | Hourly rotating files |
| `syslog_sink.h` | `syslog_sink` | System syslog |
| `systemd_sink.h` | `systemd_sink` | systemd journal |
| `systemd_namespace_sink.h` | `systemd_namespace_sink` | systemd journal namespace |
| `android_sink.h` | `android_sink` | Android logcat |
| `tcp_sink.h` | `tcp_sink` | Remote TCP server |
| `udp_sink.h` | `udp_sink` | Remote UDP endpoint |
| `callback_sink.h` | `callback_sink` | User-defined callback |
| `null_sink.h` | `null_sink` | Discard all output |
| `ostream_sink.h` | `ostream_sink` | Any `std::ostream` |
| `msvc_sink.h` | `msvc_sink` | MSVC debug output |
| `dist_sink.h` | `dist_sink` | Distribute to multiple sub-sinks |
| `dup_filter_sink.h` | `dup_filter_sink` | Suppress duplicate messages |
| `ringbuffer_sink.h` | `ringbuffer_sink` | In-memory ring buffer |
| `ansicolor_sink.h` | `ansicolor_sink` | ANSI color terminal (via `FILE*`) |
| `wincolor_sink.h` | `wincolor_sink` | Windows console color |
| `win_eventlog_sink.h` | `win_eventlog_sink` | Windows Event Log |

### Rotating File Sink Configuration

The rotating file sink (`rotating_file_sink`) supports size-based rotation:

| Parameter | Description |
|---|---|
| `filename` | Base path for the log file |
| `max_size` | Maximum file size in bytes before rotation |
| `max_files` | Number of rotated backup files to retain |

### Daily File Sink Configuration

The daily file sink (`daily_file_sink`) rotates at a specified local time:

| Parameter | Description |
|---|---|
| `filename` | Base path for the log file |
| `rotation_hour` | Hour at which rotation occurs (0–23) |
| `rotation_minute` | Minute at which rotation occurs (0–59) |

### File Event Handlers

The `file_event_handlers` structure in `common.h` defines callbacks for file lifecycle events, such as file open and close. These can be passed to file-based sinks to monitor or customize file operations.

### ANSI Color Sink Configuration

The ANSI color sink (`ansicolor_sink`) maps log severity levels to color codes for terminal output. Color schemes are defined per severity level and can be customized. The `stdout_color_sinks` and `wincolor_sink` families provide platform-appropriate color output.

![Logger and Sink Class Hierarchy](docs/class_logger_and_sink_class_hierarchy.mdd)

The diagram above shows the relationship between the `sink` interface (`common.h`), the `base_sink` template (`base_sink.h`), and the concrete sink implementations.

![File Rotation Flow](docs/sequence_file_rotation_flow.mdd)

The file rotation sequence illustrates how `rotating_file_sink` and `daily_file_sink` manage log file rotation, including backup file renaming and cleanup of old files.

For detailed usage patterns and the broader architecture, see the [Usage Guide](USAGE.md) and [Architecture](ARCHITECTURE.md). The [API Overview](API.md) provides a conceptual guide to the spdlog API, including logger creation and sink integration.