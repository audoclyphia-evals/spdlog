# Custom Formatting in spdlog

Fast, extensible pattern-based log message formatting for C++.

![Tests](https://img.shields.io/badge/tests-passing-brightgreen.svg)

spdlog provides a powerful pattern-based formatting system that controls how log messages are rendered. Users can define custom output formats using flag characters, configure the appearance of timestamps, log levels, thread identifiers, and source locations, and extend the formatter with user-defined flag handlers. The pattern formatter is the central component that parses format strings and dispatches rendering to specialized flag handlers.

## Usage

### Setting a Custom Pattern

The pattern is set on a logger instance using the `set_pattern` method. The pattern string uses percent-prefixed flag characters to insert specific pieces of information.

```cpp
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

int main() {
    // Create a file logger
    auto logger = spdlog::basic_logger_mt("file_logger", "logs/app.log");

    // Set a custom pattern: timestamp [level] message
    logger->set_pattern("%Y-%m-%d %H:%M:%S [%l] %v");

    // Log a message — output will be formatted as:
    // 2025-03-15 14:30:45 [info] Application started
    logger->info("Application started");

    // Set a pattern with source location
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%s:%#] %v");

    // Output: [2025-03-15 14:30:45.123] [file_logger] [info] [main.cpp:12] Processing data
    logger->info("Processing data");
}
```

### Common Pattern Flags

The following flag characters are available for use in pattern strings:

| Flag | Description |
|------|-------------|
| `%v` | The actual log message text |
| `%t` | Thread ID |
| `%P` | Process ID |
| `%n` | Logger name |
| `%l` | Log level (lowercase, e.g., `info`) |
| `%L` | Log level (uppercase, e.g., `INFO`) |
| `%^` | Start color range (for color sinks) |
| `%$` | End color range |
| `%s` | Source file name |
| `%#` | Source line number |
| `%!` | Source function name |
| `%o` | Elapsed time since start (stopwatch) |
| `%i` | Message sequence number |
| `%h` | Hostname |
| `%%` | Percent sign literal |

### Timestamp Formatting

Timestamp flags follow the `strftime` convention with additional sub-second precision:

```cpp
// Full date and time with milliseconds
logger->set_pattern("%Y-%m-%d %H:%M:%S.%e %v");
// Output: 2025-03-15 14:30:45.123 Hello

// Date only
logger->set_pattern("%D %v");
// Output: 03/15/25 Hello

// Time with microseconds
logger->set_pattern("%H:%M:%S.%f %v");
// Output: 14:30:45.123456 Hello

// Time with nanoseconds
logger->set_pattern("%H:%M:%S.%F %v");
// Output: 14:30:45.123456789 Hello
```

### Using the Default Logger Pattern

The global default logger's pattern can be set via the registry:

```cpp
#include "spdlog/spdlog.h"

// Set pattern on the default logger
spdlog::set_pattern("[%H:%M:%S %z] [%^%l%$] %v");

// All subsequent calls to spdlog::info, spdlog::warn, etc. use this pattern
spdlog::info("This uses the custom pattern");
spdlog::warn("Warning with colored level indicator");
```

### Custom Flag Handlers

For advanced use cases, custom flag handlers can be registered with the pattern formatter. This allows injecting user-defined content into log messages.

```cpp
#include "spdlog/spdlog.h"
#include "spdlog/pattern_formatter.h"

// Custom flag handler that inserts a formatted timestamp
class my_formatter_flag : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg &msg,
                const std::tm &tm_time,
                spdlog::memory_buf_t &dest) override {
        // Append a custom string
        dest.append("CUSTOM_FLAG");
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<my_formatter_flag>();
    }
};

int main() {
    auto logger = spdlog::stdout_color_mt("custom_logger");

    // Create a pattern formatter and register the custom flag 'Z'
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<my_formatter_flag>('Z');
    formatter->set_pattern("[%Z] %v");

    // Apply the custom formatter to the logger
    logger->set_formatter(std::move(formatter));

    // Output: [CUSTOM_FLAG] Hello from custom formatter
    logger->info("Hello from custom formatter");
}
```

The `custom_flag_formatter` base class is defined in `include/spdlog/pattern_formatter.h`. Subclasses must implement `format()` and `clone()`. The `add_flag` method associates a flag character with a custom formatter instance.

## Configuration

### Environment Variables

Logging configuration can be loaded from environment variables using the configuration helpers in `include/spdlog/cfg/env.h`.

| Variable | Description | Default | Required |
|----------|-------------|---------|----------|
| `SPDLOG_LEVEL` | Sets the global log level (e.g., `info`, `warn`, `error`, `debug`, `trace`) | `info` | No |
| `SPDLOG_PATTERN` | Sets the default log pattern for all loggers | `[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v` | No |

### Command-Line Argument Configuration

Log levels can also be configured from command-line arguments using `include/spdlog/cfg/argv.h`.

```cpp
#include "spdlog/spdlog.h"
#include "spdlog/cfg/argv.h"

int main(int argc, char *argv[]) {
    // Load log level from command-line arguments
    // Supports: --spdlog_level=debug or -spdlog_level=warn
    spdlog::cfg::load_argv_levels(argc, argv);

    // The default logger now uses the level provided via command line
    spdlog::info("This message respects the command-line log level");
}
```

### Pattern Formatter Configuration

The pattern formatter can be configured programmatically. The default pattern used by the registry is `[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v`.

```cpp
#include "spdlog/spdlog.h"
#include "spdlog/pattern_formatter.h"

// Set a new default pattern for all loggers created after this call
spdlog::set_pattern("[%n] [%^%l%$] %v");

// Create a logger — it inherits the new default pattern
auto logger = spdlog::stdout_color_mt("worker");
logger->info("Worker started");
// Output: [worker] [info] Worker started
```

### Compile-Time Tuning

The `include/spdlog/tweakme.h` header provides compile-time configuration macros that affect formatting behavior:

| Macro | Effect |
|-------|--------|
| `SPDLOG_NO_NAME` | Disables logger name in pattern (saves memory) |
| `SPDLOG_NO_SOURCE_LOC` | Disables source location tracking |
| `SPDLOG_NO_THREAD_ID` | Disables thread ID tracking |
| `SPDLOG_NO_DEFAULT_LOGGER` | Disables creation of the default logger |

### Formatting Custom Types

Custom types can be formatted in log messages by specializing the `fmt::formatter` for the type:

```cpp
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"  // For ostream-based formatting
#include "spdlog/fmt/fmt.h"

struct my_type {
    int id;
    std::string name;
};

// Specialize formatter for my_type using the fmt library
template<>
struct fmt::formatter<my_type> : fmt::formatter<std::string> {
    auto format(const my_type &obj, format_context &ctx) -> decltype(ctx.out()) {
        return fmt::format_to(ctx.out(), "[{}: {}]", obj.id, obj.name);
    }
};

int main() {
    my_type obj{42, "example"};
    spdlog::info("Object: {}", obj);
    // Output: Object: [42: example]
}
```

For types that already support `operator<<`, include `spdlog/fmt/ostr.h` to enable automatic formatting via the ostream interface.