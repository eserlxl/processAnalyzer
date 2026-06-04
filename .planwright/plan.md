# planwright Plan — .
<!-- Session: 2026-06-05T03:00:00Z -->

## Cycle 14 — Coverage + Opportunity (depth 10)

Audit found: `getProcessCpuAffinity()` only has one parser test (the range format); the
comma-list single-cpu path inside `expandCpuToken()` is untested. `currentWorkingDirectory`
field in `ProcessInfo` is only tested indirectly (via a sort test); no test directly asserts
the parsed field value. `setProcessCpuAffinity()` is documented in `api-reference.md` (line 53)
and in `features.md` but the implementation is entirely missing.

---

### Item 1 — `getProcessCpuAffinity()` comma-list format test
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/process_performance.cpp`  
**Surfaces:** `src/analyzer/performance.cpp:23-32` (`expandCpuToken`)  
**Description:** The existing `GetProcessCpuAffinityTest.ParsesRange` only exercises the
`dashPos != npos` branch. The single-element branch (no dash) and comma-list tokenization
are untested. Add `TEST(GetProcessCpuAffinityTest, ParsesCommaSeparatedList)` using
`withStatusField("Cpus_allowed_list", "0,2,4")` and asserting `cpus == {0, 2, 4}`.
This covers both the single-element branch of `expandCpuToken` and the comma-split path.  
**Verification:** Build + ctest passes. New test passes.  
**Status:** [x] done

---

### Item 2 — `currentWorkingDirectory` direct assertion
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/process_details.cpp`  
**Surfaces:** `src/analyzer/details.cpp:244-250` (cwd symlink read)  
**Description:** `currentWorkingDirectory` in `ProcessInfo` is populated from
`/proc/<pid>/cwd` (a symlink). No test directly asserts the field is populated with the
expected path — it is only exercised via a sort comparison in `query.cpp`. Extend the
existing `ProcessDetailsTest` fixture: add `withCwd(std::filesystem::temp_directory_path())`
to the `kStandalonePid` process in `SetUp()`, and add
`TEST_F(ProcessDetailsTest, GetProcessDetailsParsesCurrentWorkingDirectory)` that calls
`getProcessDetails(kStandalonePid)` and asserts
`info.currentWorkingDirectory == std::filesystem::temp_directory_path().string()`.  
**Verification:** Build + ctest passes. New test passes.  
**Status:** [x] done

---

### Item 3 — `setProcessCpuAffinity()` implementation
**Mode:** develop  
**Rung:** 3 (opportunity)  
**File:** `include/analyzer/core.h`, `src/analyzer/performance.cpp`,
  `tests/analyzer/process_performance.cpp`  
**Surfaces:** `docs/api-reference.md:53` (`setProcessCpuAffinity` documented),
  `docs/features.md:14` ("set CPU affinity programmatically"),
  `include/analyzer/system_model.h:92-95` (`CpuSet` struct),
  `src/analyzer/performance.cpp` (natural home alongside `getProcessCpuAffinity`)  
**Description:** `api-reference.md` documents `setProcessCpuAffinity(pid, affinity)` but
the method is absent from `core.h`. Implement it using `sched_setaffinity()`: convert
`CpuSet.cpus` to a `cpu_set_t` bitmask via `CPU_ZERO` / `CPU_SET`, then call
`sched_setaffinity(pid, sizeof(cpu_set_t), &mask)`. Return errors for ESRCH
(`analyzerProcessNotFound`), EPERM/EACCES (`analyzerPermissionDenied`), and EINVAL
(`analyzerParsingError`). Add declaration to `core.h` under `//- Process Performance`.
Add two tests to `tests/analyzer/process_performance.cpp`:
`SetProcessCpuAffinityTest.SetsSelfAffinityToAllCpus` (builds a `CpuSet` containing CPU 0,
calls on `getpid()`, expects success or `GTEST_SKIP` on permission denied), and
`SetProcessCpuAffinityTest.NonexistentPidReturnsError` (absent PID via MockProc).  
**Verification:** Build + ctest passes. New tests cover the new API.  
**Status:** [x] done

