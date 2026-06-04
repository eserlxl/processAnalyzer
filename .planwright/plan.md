# planwright Plan — .
<!-- Session: 2026-06-05T03:00:00Z -->

## Cycle 13 — Coverage + Opportunity seed (depth 10)

Audit found: `getPids()` has zero direct tests (both success path and `fileNotFound` path are
untested at the function boundary); `getSystemClockTicksPerSecond()` is a static method that has
never been called from a test; `getProcessCpuAffinity()` is named in `docs/features.md`, the
`CpuSet` struct already exists in `system_model.h`, and `details.cpp:156` has a comment
explicitly calling out `Cpus_allowed_list` as "add more parsing here" — making this a rung-3
seed opportunity with a real, named attachment surface.

---

### Item 1 — `getPids()` direct test coverage
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/pids.cpp` (new), CMakeLists.txt  
**Surfaces:** `src/analyzer/base.cpp:26-51` (`getPids`)  
**Description:** `getPids()` is a public API method with no direct tests — it is only exercised
indirectly through `queryProcesses()` and `snapshot()`. Two paths need coverage: (a) success —
MockProc with 3 processes, verify `getPids()` returns exactly those PIDs; (b) error — set
`procPath` to a non-existent directory, verify `getPids()` returns the `fileNotFound` error code.
Create `tests/analyzer/pids.cpp` with `TEST(PidsTest, ReturnsPidListFromMockProc)` and
`TEST(PidsTest, ReturnsFileNotFoundWhenProcPathAbsent)`. Add the new file to the test target in
`tests/CMakeLists.txt`.  
**Verification:** Build + ctest passes. Both new tests pass.  
**Status:** [x] done

---

### Item 2 — `getSystemClockTicksPerSecond()` direct test
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/system_info.cpp`  
**Surfaces:** `src/analyzer/system.cpp:49-55` (`getSystemClockTicksPerSecond`)  
**Description:** `getSystemClockTicksPerSecond()` is a static method that wraps `sysconf(_SC_CLK_TCK)`
and is exercised only indirectly inside `getProcessDetails()` CPU-time calculation. No test calls it
directly. Add `TEST(SystemClockTicksTest, ReturnsPositiveValueOnLinux)` to
`tests/analyzer/system_info.cpp` which calls `ProcessAnalyzer::getSystemClockTicksPerSecond()`
and asserts `result.has_value()` and `result.value() > 0L`.  
**Verification:** Build + ctest passes. New test passes.  
**Status:** [x] done

---

### Item 3 — `getProcessCpuAffinity()` API (rung-3 seed)
**Mode:** develop  
**Rung:** 3 (opportunity)  
**File:** `include/analyzer/core.h`, `src/analyzer/performance.cpp`,
  `tests/analyzer/process_performance.cpp`  
**Surfaces:** `src/analyzer/details.cpp:156` (comment: "Add more parsing for other fields as
needed, e.g., Cpus_allowed_list"), `include/analyzer/system_model.h:92-95` (`CpuSet` struct),
`docs/features.md:11` ("set CPU affinity programmatically via the C++ API")  
**Description:** Three signals converge: the doc names the feature, the data model exists, and
the parser comment explicitly names the field to parse. Implement
`getProcessCpuAffinity(int pid) -> Result<CpuSet>` which reads `/proc/<pid>/status`, finds the
`Cpus_allowed_list:` line, and parses the range-list format (e.g., `"0-3"` → `{0,1,2,3}`;
`"0,2"` → `{0,2}`; `"0-1,3"` → `{0,1,3}`). Return `analyzerParsingError` when the field is
missing. Add the declaration to `core.h` under `//- Process Control`. Implement in
`src/analyzer/performance.cpp`. Add three tests to `tests/analyzer/process_performance.cpp`:
`GetProcessCpuAffinityParsesRange` (`withStatusField("Cpus_allowed_list", "0-3")` → cpus
`{0,1,2,3}`), `GetProcessCpuAffinityAbsentPidReturnsError` (nonexistent PID),
`GetProcessCpuAffinityMissingFieldReturnsError` (process exists but no `Cpus_allowed_list`
in status).  
**Verification:** Build + ctest passes. Three new tests cover the new API.  
**Status:** [x] done

