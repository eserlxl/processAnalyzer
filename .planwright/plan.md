# planwright Plan — .
<!-- Session: 2026-06-05T01:00:00Z -->

## 1. [improve] Integration tests for `cmdlineContains` filter in `queryProcesses`

**Surfaces:** `tests/analyzer/query.cpp` (`QueryProcessesTest` fixture or a new fixture)

**Evidence:** `ProcessFilter::cmdlineContains` is applied in `passesStaticFilters` and wired to `--cmdline` in Cycle 5, but `queryProcesses` is never called with `cmdlineContains` set in any test. The CLI-to-filter-to-query path is only partially verified.

**Development:** Add two tests using the existing `QueryProcessesTest` fixture (or a small inline mock): `FilterByCmdlineMatchReturnsSubset` — set `filter.cmdlineContains = "alpha"` and assert only the "alpha" processes (kPidAlphaA, kPidAlphaB) are returned; `FilterByCmdlineNoMatchReturnsEmpty` — set a pattern that matches nothing and assert an empty result. If the default cmdline from `withName` equals the process name, the "alpha" substring is sufficient; otherwise extend setup with `.withCmdlineArgs({"alpha", "--flag"})`.

**Verification:** Build and `ctest` pass. Both new tests pass.

---

## 2. [improve] Integration tests for virtual memory range filter in `queryProcesses`

**Surfaces:** `tests/analyzer/query.cpp`

**Evidence:** `ProcessFilter::minVirtualMemoryKB` / `maxVirtualMemoryKB` are implemented and wired in Cycle 5, but never exercised end-to-end. A regression in the wiring would be invisible.

**Development:** Add a small fixture (or extend `QueryProcessesTest`) that creates two processes with different virtual memory values using `buildProcess(...).withStat(...)` (set `statData.vsize`). Add `FilterByMinVmExcludesSmall` and `FilterByMaxVmExcludesLarge` tests. Note: `vsize` is in bytes in the stat struct; the filter compares against KB (divide by 1024).

**Verification:** Build and `ctest` pass.

---

## 3. [improve] Integration tests for priority range filter in `queryProcesses`

**Surfaces:** `tests/analyzer/query.cpp`

**Evidence:** `ProcessFilter::minPriority` / `maxPriority` are implemented and wired, but never exercised end-to-end.

**Development:** Add a small fixture that creates two processes with different priority values via `buildProcess(...).withStat(...)` (set `statData.priority`). Add `FilterByMinPriorityExcludesLow` and `FilterByMaxPriorityExcludesHigh` tests. Note: the analyzer stores `info.priority = static_cast<int>(priorityL)` where `priorityL` is field 18 of `/proc/pid/stat` (kernel priority, not nice value).

**Verification:** Build and `ctest` pass.

---

## 4. [improve] Add "missing maps file → error" test to `GetProcessMemoryMapsTest`

**Surfaces:** `tests/analyzer/memory_maps.cpp`

**Evidence:** Every other thin test suite (`GetProcessCgroupInfoTest`, `GetProcessResourceLimitsTest`) has a "process dir exists but the file is absent → returns error" test. `GetProcessMemoryMapsTest` has only 2 tests and is missing this case, breaking the pattern.

**Development:** Add `TEST_F(GetProcessMemoryMapsTest, MissingMapsFileReturnsError)` — call `analyzer.getProcessMemoryMaps(kPid)` without creating the maps file first (the fixture's `SetUp` calls `createMaps` in the happy-path test, but the `kPid` directory exists). The result must not have a value.

**Verification:** Build and `ctest` pass.

---

## 5. [improve] Add UDP connection test to `GetNetworkConnectionsTest`

**Surfaces:** `tests/analyzer/network_connections.cpp`

**Evidence:** `getNetworkConnections` parses `net/tcp`, `net/tcp6`, `net/udp`, and `net/udp6`. Only `net/tcp` is created in the existing test fixture. A bug in UDP parsing would be invisible.

**Development:** Extend `GetNetworkConnectionsTest::SetUp` to also create a `net/udp` file with one UDP entry whose inode matches a second socket fd on `kMatchingPid`. Add test `ParsesMatchingUdpConnection` — call `getNetworkConnections(kMatchingPid)`, assert the result contains a connection with `protocol == "UDP"`. Alternatively, create a separate fixture if adding to `SetUp` would break the existing tests.

**Verification:** Build and `ctest` pass. Both existing tests and the new UDP test pass.

---

## 6. [develop] Add `--env` flag to `show` command: print process environment variables

**Surfaces:** `include/cli/args.h` (`ParsedArguments`), `src/cli/args.cpp`, `src/main.cpp`

**Evidence:** `getProcessEnvironment(pid)` is implemented and returns `vector<string>` of `KEY=VALUE` entries. No CLI flag exposes it. The `show` command has an established subsection pattern (`--children`, `--threads`, `--open-files`, `--network`) that this slot fits naturally.

**Development:** Add `bool showEnv = false;` to `ParsedArguments`. In `args.cpp`, add `--env` / `--environment` parsing. In `main.cpp`, in the `show`/`pid` single-process path (before the `return 0` at line 269), add a block identical in structure to the `--threads` block: call `getProcessEnvironment(targetPid)`, print a `"\nEnvironment Variables:\n"` header, then one line per `KEY=VALUE` entry. Add usage line. Add test `EnvFlagIsSet` in `tests/cli/args.cpp`.

**Verification:** Build and `ctest` pass. Smoke: `processAnalyzer show --pid 1 --env` prints environment entries.

---

## 7. [develop] Add `--maps` flag to `show` command: print process memory maps

**Surfaces:** `include/cli/args.h`, `src/cli/args.cpp`, `src/main.cpp`

**Evidence:** `getProcessMemoryMaps(pid)` is implemented and returns `vector<MemoryMapInfo>`. No CLI flag exposes it. Follows the `show` subsection pattern.

**Development:** Add `bool showMemoryMaps = false;` to `ParsedArguments`. Parse `--maps` in `args.cpp`. In `main.cpp`, add a block that calls `getProcessMemoryMaps(targetPid)` and prints a table with columns: address range (`startAddress`–`endAddress`), permissions, pathname (or `[anon]` if empty). Use fixed-width formatting consistent with the existing threads/files output. Add usage line. Add test `MapsFlagIsSet` in `tests/cli/args.cpp`.

**Verification:** Build and `ctest` pass. Smoke: `processAnalyzer show --pid 1 --maps` prints memory map sections.

---

## 8. [improve] Add `customPredicate` test to `QueryProcessesTest`

**Surfaces:** `tests/analyzer/query.cpp`

**Evidence:** `ProcessFilter::customPredicate` (an `optional<ProcessPredicate>`) is applied at line 44 of `filter_helpers.cpp` but has zero test coverage. Any regression in its evaluation would be silent.

**Development:** In `tests/analyzer/query.cpp`, add `TEST_F(QueryProcessesTest, CustomPredicateFiltersProcesses)` — set `filter.customPredicate` to a lambda that returns `info.pid <= kPidBravo` (i.e., only accept PIDs ≤ 20). Call `queryProcesses(filter, ...)`. Assert the result contains only kPidAlphaA (10) and kPidBravo (20), not kPidAlphaB (30) or kPidCharlie (40).

**Verification:** Build and `ctest` pass. The new test passes.
