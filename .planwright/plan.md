# planwright Plan — .
<!-- Session: 2026-06-05T02:00:00Z -->

## 1. [improve] Integration test for `nameRegex` filter in `queryProcesses`

**Surfaces:** `tests/analyzer/query.cpp` (`QueryProcessesTest` fixture)

**Evidence:** `ProcessFilter::nameRegex` is applied in `passesStaticFilters` (filter_helpers.cpp:33 via `applyStringFilter`) but no test ever calls `queryProcesses` with `nameRegex` set. The regex code path is exercised by zero tests.

**Development:** In `QueryProcessesTest`, add `FilterByNameRegexMatchesSubset` — set `filter.nameRegex = std::regex("^alpha$")` and assert only the two "alpha" PIDs (kPidAlphaA, kPidAlphaB) are returned. Add `FilterByNameRegexNoMatch` — set a pattern that matches nothing and assert empty result.

**Verification:** Build and `ctest` pass.

---

## 2. [improve] Test `NetworkFilterCriteria::state` filter in `queryProcesses`

**Surfaces:** `tests/analyzer/query.cpp` (`QueryNetworkFilterTest` fixture)

**Evidence:** `passesNetworkFilter` at line 55 checks `netFilter.state`, but this field is set by no test. The existing TCP connection in the fixture has `state == "ESTABLISHED"` (hex `01`). Filtering by state can be tested with that existing mock.

**Development:** Add `FilterByConnectionStateMatches` — set `netFilter.state = "ESTABLISHED"`, assert `kNetPid` is returned. Add `FilterByConnectionStateNoMatch` — set `netFilter.state = "LISTEN"`, assert empty result.

**Verification:** Build and `ctest` pass.

---

## 3. [improve] Test `NetworkFilterCriteria::protocol` filter in `queryProcesses`

**Surfaces:** `tests/analyzer/query.cpp` (`QueryNetworkFilterTest` fixture or new fixture)

**Evidence:** `passesNetworkFilter` line 54 checks `netFilter.protocol`, but zero tests exercise this field. A TCP-only process should pass `protocol == "TCP"` and fail `protocol == "UDP"`.

**Development:** Add `FilterByProtocolTcpMatches` — set `netFilter.protocol = "TCP"`, assert `kNetPid` is returned. Add `FilterByProtocolUdpNoMatch` — set `netFilter.protocol = "UDP"`, assert empty result (no UDP entry in the fixture's `net/tcp`).

**Verification:** Build and `ctest` pass.

---

## 4. [improve] Validate that inspection flags are rejected with `list` command

**Surfaces:** `tests/cli/args.cpp`

**Evidence:** The validation block in `args.cpp` (line 425) rejects `--env` and `--maps` when used with `list`, but there are zero tests for this rejection. The pattern is broken — `--children`, `--threads`, `--open-files`, `--network` have no validation tests either.

**Development:** Add four tests in `tests/cli/args.cpp`: `EnvFlagWithListCommandReturnsNullopt` (`list --env` → nullopt), `MapsFlagWithListCommandReturnsNullopt` (`list --maps` → nullopt), `ChildrenFlagWithListCommandReturnsNullopt` (`list --children` → nullopt), `ThreadsFlagWithListCommandReturnsNullopt` (`list --threads` → nullopt). Use `makeArgv({"processAnalyzer", "list", "--env"})` etc.

**Verification:** Build and `ctest` pass. All four new tests pass.

---

## 5. [improve] Test `executablePathContains` integration filter in `queryProcesses`

**Surfaces:** `tests/analyzer/query.cpp`

**Evidence:** `ProcessFilter::executablePathContains` is applied at `filter_helpers.cpp:35` but no test ever sets it and calls `queryProcesses`. Processes in `QueryProcessesTest` have no `executablePath` set (no exe symlink). A new fixture is needed.

**Development:** Add a small fixture with two processes that have distinct exe paths (e.g., `/usr/bin/server` and `/usr/lib/helper`) via `buildProcess(...).withExe(mockFilePath).create()`. Add `FilterByExecPathMatchReturnsSubset` and `FilterByExecPathNoMatchReturnsEmpty`.

**Verification:** Build and `ctest` pass.

---

## 6. [develop] Add `--limits` flag to `show` command: print resource limits

**Surfaces:** `include/cli/args.h`, `src/cli/args.cpp`, `src/main.cpp`

**Evidence:** `getProcessResourceLimits(pid)` is implemented and returns `ResourceLimitInfo` with a `limits` vector (resource, softLimit, hardLimit, units). No CLI flag exposes it. Follows the established `show` subsection pattern.

**Development:** Add `bool showLimits = false;` to `ParsedArguments`. Parse `--limits` in `args.cpp` and add to the inspection-flag validation block. In `main.cpp`, add a block calling `getProcessResourceLimits(targetPid)` and print a table: Limit name, Soft Limit, Hard Limit, Units. Add usage line and a `LimitsFlagIsSet` test in `tests/cli/args.cpp`.

**Verification:** Build and `ctest` pass.

---

## 7. [develop] Add `--cgroup` flag to `show` command: print cgroup membership

**Surfaces:** `include/cli/args.h`, `src/cli/args.cpp`, `src/main.cpp`

**Evidence:** `getProcessCgroupInfo(pid)` is implemented and returns `CgroupInfo` with entries (id, controllers, path). No CLI flag exposes it. Follows the `show` subsection pattern.

**Development:** Add `bool showCgroupInfo = false;` to `ParsedArguments`. Parse `--cgroup` in `args.cpp` and add to the inspection-flag validation block. In `main.cpp`, add a block printing each cgroup entry as `id:controllers:path`. Add usage line and a `CgroupFlagIsSet` test.

**Verification:** Build and `ctest` pass.

---

## 8. [improve] Add edge-case tests to `GetProcessEnvironmentTest`

**Surfaces:** `tests/analyzer/environment.cpp`

**Evidence:** Only 2 tests exist: happy path (2 vars) and absent pid. Missing: (a) env var whose value contains `=` (e.g., `PATH=/usr/bin:/bin`) — the split on `\0` is correct but it's never verified that `=` in the value is preserved; (b) empty environ file → empty vector returned (not an error).

**Development:** Add `ReturnsEntryWithEqualsInValue` — create a process with an environ containing `PATH=/usr/bin:/bin`, call `getProcessEnvironment`, assert that the entry `"PATH=/usr/bin:/bin"` (the whole string including the `=` in the value) is in the result. Add `EmptyEnvironReturnsEmptyVector` — create a process with an empty `environ` file and assert the result is a non-error empty vector.

**Verification:** Build and `ctest` pass.
