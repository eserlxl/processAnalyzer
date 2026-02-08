# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Changed
- Refactored `Utils` library to use `std::expected` (as `Result<T>`) for robust error handling.
- Deprecated older utility functions using `std::optional` or out-parameters for errors.
- Enhanced `docs/UTILS.md` to reflect the new API and error handling patterns.
- Added `TestUtilsNewApiTest` for verifying the new Utility API.

## [0.1.0] - 2026-02-08

### Added
- Initial release of `processAnalyzer`.
- Feature: List all running processes.
- Feature: Get detailed information for a specific process.
- Feature: Filter processes by name and user.
- Feature: Sort processes by various fields.
- Feature: Customizable output columns and formats (CSV, JSON).
- Feature: Utility library for file system, string, and numeric operations.
- Added unit tests for core functionalities.
- Added comprehensive documentation.
