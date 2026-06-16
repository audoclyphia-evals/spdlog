# spdlog Architecture

![Tests](https://img.shields.io/badge/tests-passing-brightgreen.svg)

Fast C++ logging library providing a modular architecture for thread-safe log output with support for multiple sinks, custom formatting, and asynchronous logging.

## Architecture

spdlog is built around four primary components that work together to process log messages:

- **Loggers** – The public API through which applications send log messages. Each logger holds a name, a log level, a formatter, and a collection of sinks. The `logger` class (defined in `include/spdlog/logger.h`) manages level filtering, formatting, and dispatching messages to its sinks.
- **Sinks** – Output destinations for log messages. Each sink implements the `sink` interface (defined in `include/spdlog/sinks/sink.h`) and receives fully formatted messages. The library provides a variety of sink implementations for different outputs; see the [Sinks Reference](SINKS.md) for details.
- **Formatters** – Responsible for converting log messages into string output. The default `pattern_formatter` (in `include/spdlog/pattern_formatter.h`) supports configurable patterns with flags for time, level, logger name, message, and more. Custom formatters can be created by implementing the `formatter` interface. See [Custom Formatting](CUSTOM_FORMATTING.md) for details.
- **Registry** – A central registry (in `include/spdlog/details/registry.h`) that manages all logger instances, provides global configuration, and handles periodic flushing and thread pool lifecycle. It creates a default logger on initialization.

Messages flow through the system as follows:

1. An application calls a logging method on a logger (e.g., `logger->info("message")`).
2. The logger checks the message level against its configured threshold.
3. If the level passes, the logger formats the message using its formatter, producing a string.
4. The formatted message is passed to each attached sink, which writes it to its output destination.
5. For asynchronous loggers, the message is posted to a thread pool queue instead of being processed immediately. A background worker dequeues and forwards the message to the sinks.

The architecture supports both synchronous and asynchronous modes. Synchronous logging blocks the calling thread until the sink writes the message. Asynchronous logging uses a multi-producer, multi-consumer blocking queue (`mpmc_blocking_q`) and a thread pool (`thread_pool`) to decouple the logging call from the I/O operation, reducing latency in the calling thread.

The following diagrams illustrate the key components and their interactions:

```mermaid
flowchart TB
    %% ===== User Tier =====
    subgraph User_Tier [User Interaction]
        user([User])
    end

    %% ===== API / Factory Tier =====
    subgraph API_Layer [Public API]
        api[spdlog.h]
        factory[Sync Factory]
    end

    %% ===== Core Components =====
    subgraph Core_Components [Core Components]
        logger[Logger]
        registry[Registry]
        formatter[Pattern Formatter]
        sink[Base Sink]
    end

    %% ===== Async Infrastructure =====
    subgraph Async_Infrastructure [Async Infrastructure]
        async_logger[Async Logger]
        thread_pool[Thread Pool]
    end

    %% ===== Sink Implementations =====
    subgraph Sink_Implementations [Sink Implementations]
        file_sinks[File Sinks]
        console_sinks[Console Sinks]
    end

    %% ===== Output Destinations =====
    subgraph Output_Destinations [Output Destinations]
        file((File))
        console((Console))
    end

    %% ===== Relationships =====
    user -->|log API calls| api
    api -->|creates via| factory
    api -->|creates via async.h| async_logger
    factory -->|creates| logger

    registry -->|manages| logger
    registry -->|configures| formatter
    registry -->|configures| sink

    logger -->|writes to| sink
    logger -->|formats with| formatter

    async_logger -->|extends| logger
    async_logger -->|posts to| thread_pool
    thread_pool -->|writes to| sink

    sink -.-> file_sinks
    sink -.-> console_sinks

    file_sinks -->|output| file
    console_sinks -->|output| console
```

```mermaid
classDiagram
    direction TB

    namespace sinks {
        class sink {
            <<Interface>>
            +log(msg: log_msg): void
            +flush(): void
            +set_pattern(pattern: string): void
            +set_formatter(formatter: unique_ptr): void
        }
        class rotating_file_sink {
            +rotating_file_sink(filename, max_size, max_files)
            +log(msg: log_msg): void
            +flush(): void
        }
        class daily_file_sink {
            +daily_file_sink(filename, rotation_hour, rotation_minute)
            +log(msg: log_msg): void
            +flush(): void
        }
        class stdout_sink_base {
            <<Abstract>>
            +log(msg: log_msg): void
            +flush(): void
        }
        class syslog_sink {
            +syslog_sink(ident, option, facility)
            +log(msg: log_msg): void
            +flush(): void
        }
        class tcp_sink {
            +tcp_sink(host, port)
            +log(msg: log_msg): void
            +flush(): void
        }
        class udp_sink {
            +udp_sink(host, port)
            +log(msg: log_msg): void
            +flush(): void
        }
        class kafka_sink {
            +kafka_sink(topic, config)
            +log(msg: log_msg): void
            +flush(): void
        }
        class ostream_sink {
            +ostream_sink(stream)
            +log(msg: log_msg): void
            +flush(): void
        }
        class systemd_sink {
            +systemd_sink()
            +log(msg: log_msg): void
            +flush(): void
        }
    }

    namespace async {
        class async_logger {
            +async_logger(name, sink, tp)
            +log(msg: log_msg): void
            +flush(): void
        }
        class thread_pool {
            +thread_pool(q_size, thread_count)
            +post_log(msg): void
            +post_flush(worker): void
        }
        class async_factory_impl {
            +create(logger_name, sink_args): shared_ptr
        }
    }

    namespace core {
        class logger {
            <<Abstract>>
            +log(level, msg): void
            +flush(): void
            +set_level(level): void
            +sinks(): vector~sink_ptr~
        }
        class registry {
            +instance(): registry
            +create(logger_name): shared_ptr
            +get(logger_name): shared_ptr
            +set_default_logger(logger): void
        }
        class file_event_handlers {
            +before_open: function
            +after_open: function
            +before_close: function
            +after_close: function
        }
    }

    rotating_file_sink ..|> sink : implements
    daily_file_sink ..|> sink : implements
    stdout_sink_base ..|> sink : implements
    syslog_sink ..|> sink : implements
    tcp_sink ..|> sink : implements
    udp_sink ..|> sink : implements
    kafka_sink ..|> sink : implements
    ostream_sink ..|> sink : implements
    systemd_sink ..|> sink : implements

    async_logger --> thread_pool : uses
    async_factory_impl --> async_logger : creates

    logger --> sink : has > 1

    registry --> logger : manages

    async_logger --|> logger : extends

    rotating_file_sink --> file_event_handlers : may use
    daily_file_sink --> file_event_handlers : may use

    async_factory_impl --> thread_pool : uses
```

![Formatter and Pattern Formatter Classes](docs/class_formatter_and_pattern_formatter_classes.mmd)

```mermaid
classDiagram
    direction TB

    %% Grouping by namespace (flat, no nesting)
    namespace spdlog {
        class async_factory_impl {
            <<Factory>>
            +create(logger_name, args...): shared_ptr~async_logger~
        }
        class async_logger {
            %% Fast asynchronous logger using pre‑allocated queue and a single back thread
        }
    }

    namespace details {
        class thread_pool {
            %% Thread pool that owns a queue and processes async messages
        }
        class async_msg {
            %% Structure representing an asynchronous log message for queueing
        }
        class mpmc_blocking_queue {
            %% Thread‑safe blocking queue for multi‑producer/multi‑consumer
        }
        class circular_q {
            %% Fixed‑size circular queue (ring buffer) using vector storage
        }
    }

    %% Relationships (outside all namespace blocks)
    async_factory_impl --> async_logger : creates
    async_logger --> thread_pool : posts messages to
    thread_pool --> mpmc_blocking_queue : manages
    mpmc_blocking_queue --> circular_q : uses

    %% The async_msg type flows through the queue
    async_logger ..> async_msg : sends
    thread_pool ..> async_msg : processes
```

```mermaid
sequenceDiagram
    autonumber
    actor User
    participant spdlog as "spdlog::info"
    participant Logger as "spdlog::logger"
    participant Sink as "sink (interface)"

    User->>+spdlog: info(fmt, args...)
    spdlog->>+Logger: info(fmt, args...)
    
    Logger->>Logger: log(level::info, fmt, args...)
    note right of Logger: Level check & format message using fmt library
    
    opt Level enabled
        Logger->>Logger: sink_it_(log_msg)
        Logger->>+Sink: log(log_msg)
        Sink->>Sink: format & write to output
        Sink-->>-Logger: (return)
        Logger-->>-spdlog: (return)
        spdlog-->>-User: (return)
    end
```

```mermaid
sequenceDiagram
    autonumber
    participant User as "User Code"
    participant AsyncLogger as "async_logger"
    participant ThreadPool as "thread_pool"
    participant Queue as "mpmc_blocking_queue"
    participant Worker as "Worker Thread"
    participant Sink as "Backend Sink"

    %% Log message flow - async_logger::sink_it_() posts to thread_pool
    User->>+AsyncLogger: info("Async message #{}", i)
    note over AsyncLogger: sink_it_(msg)
    AsyncLogger->>+ThreadPool: post_log(shared_from_this(), msg, overflow_policy)
    note over ThreadPool: Creates async_msg(type=log)
    ThreadPool->>+Queue: enqueue(async_msg)
    Queue-->>-ThreadPool: ok
    ThreadPool-->>-AsyncLogger: return
    AsyncLogger-->>-User: return

    %% Worker thread processes queue messages
    loop Process messages
        Worker->>+Queue: dequeue()
        Queue-->>-Worker: async_msg
        alt msg type is log
            Worker->>+Sink: log(msg)
            Sink-->>-Worker: return
        else msg type is flush
            Worker->>+Sink: flush()
            Sink-->>-Worker: return
        else msg type is terminate
            Worker->>Worker: exit loop
        end
    end

    %% Flush flow - async_logger::flush_() posts flush to thread_pool
    User->>+AsyncLogger: flush()
    note over AsyncLogger: flush_(msg)
    AsyncLogger->>+ThreadPool: post_flush(shared_from_this(), overflow_policy)
    note over ThreadPool: Creates async_msg(type=flush)
    ThreadPool->>+Queue: enqueue(async_msg)
    Queue-->>-ThreadPool: ok
    ThreadPool-->>-AsyncLogger: return
    AsyncLogger-->>-User: return
```

```mermaid
sequenceDiagram
    participant RS as "rotating_file_sink" %% source: Context #2 row 3
    participant DS as "daily_file_sink" %% source: Context #2 row 1
    participant HS as "hourly_file_sink" %% source: Context #2 row 2

    Note over RS,HS: Log message arrives at sink via sink_it_()

    par Size-based rotation flow
        activate RS
        RS->>RS: Check file size vs max_size
        alt Size exceeds max_size
            RS->>RS: rotate_now()
            RS->>RS: calc_filename(index)
            Note over RS: Rename current file#59;<br/>open new file
            RS->>RS: Cleanup old files > max_files
        end
        deactivate RS
    and Daily rotation flow
        activate DS
        DS->>DS: Check current time vs rotation_hour/minute
        alt Time matches rotation window
            Note over DS: Rename current file with date stamp
            DS->>DS: Cleanup old files > max_files
        end
        deactivate DS
    and Hourly rotation flow
        activate HS
        HS->>HS: Check hour change since last rotation
        alt New hour detected
            Note over HS: Rename current file with date-hour stamp
            HS->>HS: Cleanup old files > max_files
        end
        deactivate HS
    end
```

```mermaid
sequenceDiagram
    autonumber
    %% Participants grounded in context #3
    box "User Code"
        actor User
    end
    box "spdlog API"
        participant spdlog as "spdlog namespace"
    end
    box "Core Registry"
        participant Registry as "details::registry"
        participant Logger as "Logger instance"
    end
    box "Internal Resources"
        participant PeriodicFlusher
        participant ThreadPool
    end

    %% 1. Configure automatic registration
    User->>spdlog: set_automatic_registration(false)
    spdlog->>+Registry: set_automatic_registration(false)
    Registry->>Registry: lock logger_map_mutex_
    Registry-->>-spdlog: 

    %% 2. Register a logger
    User->>spdlog: register_logger(logger)
    spdlog->>+Registry: register_logger(logger)
    Registry->>Registry: lock logger_map_mutex_
    Registry->>Registry: register_logger_(new_logger)
    Registry-->>-spdlog: 

    %% 3. Apply global flush level
    User->>spdlog: flush_on(level::err)
    spdlog->>+Registry: flush_on(err)
    Registry->>Registry: lock logger_map_mutex_
    loop over each logger
        Registry->>Logger: flush_on(err)
    end
    Registry-->>-spdlog: 

    %% 4. Flush all loggers (orderly shutdown preparation)
    User->>spdlog: flush_all()
    spdlog->>+Registry: flush_all()
    Registry->>Registry: lock logger_map_mutex_
    loop over each logger
        Registry->>Logger: flush()
    end
    Registry-->>-spdlog: 

    %% 5. Shutdown the registry
    User->>spdlog: shutdown()
    spdlog->>+Registry: shutdown()
    Registry->>Registry: lock flusher_mutex_
    Registry->>PeriodicFlusher: reset()
    Registry->>Registry: drop_all()
    Registry->>Registry: lock logger_map_mutex_
    Registry->>Registry: clear loggers_
    Registry->>Registry: reset default_logger_
    Registry->>Registry: lock tp_mutex_
    Registry->>ThreadPool: reset()
    Registry-->>-spdlog: 

    note over Registry: Shutdown complete
```

## Project Structure

```
spdlog/
├── include/spdlog/       # Core headers (public API)
│   ├── cfg/              # Configuration from environment and command-line arguments
│   ├── details/          # Internal implementation (registry, thread pool, file helpers, etc.)
│   ├── fmt/              # Formatting support (bundled fmt library and extensions)
│   ├── sinks/            # Output sink implementations (file, console, syslog, network, etc.)
│   ├── async.h           # Async logger factory
│   ├── async_logger.h    # Async logger class
│   ├── common.h          # Core types, log levels, source location
│   ├── formatter.h       # Formatter interface
│   ├── logger.h          # Logger class
│   ├── mdc.h             # Mapped Diagnostic Context
│   ├── pattern_formatter.h # Pattern-based formatter
│   ├── spdlog.h          # Main public API header
│   ├── stopwatch.h       # Stopwatch utility
│   └── version.h         # Version macros
├── src/                  # Compiled source files (for non-header-only builds)
├── example/              # Example usage code
├── tests/                # Test suite (Catch2-based)
├── bench/                # Performance benchmarks
└── scripts/              # Utility scripts (e.g., version extraction)
```

The `include/spdlog` directory contains all public headers. The `details/` subdirectory holds internal machinery such as the registry, thread pool, file helper, and OS utilities. The `sinks/` subdirectory provides a wide range of output destinations; for a complete list and configuration details, see the [Sinks Reference](SINKS.md). The `fmt/` directory includes the bundled fmt library and spdlog-specific formatting extensions (e.g., `bin_to_hex.h`, `chrono.h`). The `src/` directory contains compiled implementations for components that benefit from separate compilation (e.g., async logger, file sinks, color sinks). The `example/` directory demonstrates common usage patterns, and `tests/` provides comprehensive test coverage. For contribution guidelines, see [Contributing to spdlog](CONTRIBUTING.md). For a high-level API overview, see the [API Overview](API.md).