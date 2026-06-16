# Contributing to spdlog

![Tests](https://img.shields.io/badge/tests-passing-brightgreen.svg)

Fast C++ logging library. spdlog provides a high-performance, header-only logging framework with support for synchronous and asynchronous logging, multiple sink types, and customizable formatting.

## Development

### Environment Setup

spdlog is a header-only C++ library. To set up a development environment:

1.  Ensure a C++17 compatible compiler is installed (GCC 7+, Clang 5+, MSVC 2017+).
2.  Clone the repository.
3.  The library is located in the `include/` directory. No separate build step is required for header-only usage.

For building examples, benchmarks, and tests, CMake is required:

```bash
# Configure the project
cmake -S . -B build

# Build examples
cmake --build build --target example

# Build benchmarks
cmake --build build --target bench
```

### Code Structure

The source tree is organized as follows:

- `include/spdlog/` — Public headers (core library, sinks, formatters, details).
- `src/` — Compiled source files for non-header-only mode.
- `example/` — Usage examples.
- `bench/` — Performance benchmarks.
- `tests/` — Test suite.
- `scripts/` — Utility scripts.

### Code Style

- Follow the existing code style in the repository.
- Use meaningful names for classes, functions, and variables.
- Ensure thread safety for any new sink or component.
- Use `SPDLOG_INLINE` for inline function definitions in header files.
- Use `SPDLOG_API` for symbol visibility annotations on exported classes.

## Testing

### Running Tests

The test suite uses the Catch2 framework. To build and run tests:

```bash
# Configure with tests enabled
cmake -S . -B build -DSPDLOG_BUILD_TESTS=ON

# Build tests
cmake --build build --target tests

# Run all tests
cd build && ctest
```

### Test Structure

- Test files are located in the `tests/` directory.
- The main test entry point is `tests/main.cpp`.
- Test files follow the naming convention `test_<feature>.cpp` (e.g., `test_async.cpp`, `test_file_logging.cpp`).
- Common test utilities are in `tests/utils.h` and `tests/utils.cpp`.
- A shared test sink is defined in `tests/test_sink.h`.

### Writing New Tests

1.  Create a new file `tests/test_<feature>.cpp`.
2.  Include `tests/includes.h` for common headers.
3.  Use Catch2 `TEST_CASE` and `REQUIRE` macros.
4.  Add the test file to the CMake build configuration.

## Contributing

### Pull Request Workflow

1.  Fork the repository on GitHub.
2.  Create a feature branch from `main` or `v2.x`.
3.  Make changes following the code style guidelines.
4.  Add or update tests to cover the changes.
5.  Ensure all existing tests pass.
6.  Submit a pull request with a clear description of the changes.

### Commit Messages

- Use clear, descriptive commit messages.
- Prefix commits with the component being changed (e.g., `[sinks]`, `[formatter]`, `[async]`).
- Reference issue numbers when applicable.

### Reporting Issues

- Use the GitHub issue tracker to report bugs or request features.
- Include a minimal reproducible example for bug reports.
- Specify the spdlog version, compiler, and operating system.

### Code Review

- All pull requests require review before merging.
- Address review feedback promptly.
- Keep pull requests focused on a single change or feature.