# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Add rate_limit_sink class in spdlog to implement rate-limiting for log messages, allowing users to set a maximum message count per time window. (rate_limit_sink.h)

### Changed

- Update the dup_filter_sink class to properly handle log levels when filtering duplicate messages within a time window. (tcp_sink.h, dup_filter_sink.h)
