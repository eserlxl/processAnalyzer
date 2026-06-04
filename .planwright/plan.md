# planwright Plan — .
<!-- Session: 2026-06-05T03:00:00Z -->

## Cycle 16 — Coverage + Opportunity (depth 10)

Audit found: `getSystemCpuStats()` has no test for the malformed-cpu-line error path (when
the `cpu` line exists but has fewer than 8 fields, the parse fails — untested).
`api-reference.md` has a duplicate `getSystemCpuStats()` entry (once in "System-wide
Information & Statistics", again in "System Performance Metrics"). `getSystemCpuUsage(duration)`
is documented in `api-reference.md:70`, `SystemCpuUsage` struct exists in `system_model.h:116`,
but the implementation is absent from `core.h`.

---

### Item 1 — `getSystemCpuStats()` malformed line error path
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/system_info.cpp`  
**Surfaces:** `src/analyzer/system.cpp:125-132` (parse failure branch inside `getSystemCpuStats`)  
**Description:** `getSystemCpuStats()` returns `analyzerParsingError` when the `cpu` line
has fewer than 8 parseable fields, but this branch is untested. Add
`TEST(SystemCpuStatsTest, MalformedCpuLineReturnsError)` — create a mock `stat` file with
`"cpu  100 20\n"` (only 2 fields after the label, far fewer than the 8 required), and assert
`result.has_value() == false`.  
**Verification:** Build + ctest passes. New test passes.  
**Status:** [x] done

---

### Item 2 — `getSystemCpuUsage(duration)` implementation
**Mode:** develop  
**Rung:** 3 (opportunity)  
**File:** `include/analyzer/core.h`, `src/analyzer/system.cpp`,
  `tests/analyzer/system_info.cpp`  
**Surfaces:** `docs/api-reference.md:70` (`getSystemCpuUsage(duration)` documented),
  `include/analyzer/system_model.h:116-119` (`SystemCpuUsage` struct with `cpuPercentage`),
  `src/analyzer/system.cpp` (natural home alongside `getSystemCpuStats`)  
**Description:** Implement `getSystemCpuUsage(std::chrono::milliseconds duration) ->
Result<SystemCpuUsage>`: call `getSystemCpuStats()` twice (before and after
`std::this_thread::sleep_for(duration)`), compute the active delta
(user+nice+system+irq+softirq+steal) divided by total delta, multiply by 100, and return
a `SystemCpuUsage`. If either snapshot fails, propagate the error; if total delta is zero
(clock did not advance), return 0.0%. Add declaration to `core.h`. Add two tests to
`tests/analyzer/system_info.cpp`:
`TEST(SystemCpuUsageTest, ReturnsPercentageInRange)` — call with
`std::chrono::milliseconds(1)` and assert `0.0 <= result.value().cpuPercentage <= 100.0`;
`TEST(SystemCpuUsageTest, MissingStatReturnsError)` — absent `stat` file, expect error.  
**Verification:** Build + ctest passes. New tests cover the new API.  
**Status:** [x] done

---

### Item 3 — Fix duplicate `getSystemCpuStats()` in api-reference.md
**Mode:** docs  
**Rung:** 2 (coverage)  
**File:** `docs/api-reference.md`  
**Surfaces:** `docs/api-reference.md:66-70` (duplicate entry)  
**Description:** Cycle 15 added `getSystemCpuStats()` to "System-wide Information &
Statistics" but left the original stub entry in "System Performance Metrics" as well.
Remove the duplicate entry from "System Performance Metrics". Also add the newly
implemented `getSystemCpuUsage(duration)` to "System Performance Metrics" once Item 2
is done.  
**Verification:** `api-reference.md` has exactly one `getSystemCpuStats()` entry.  
**Status:** [x] done

