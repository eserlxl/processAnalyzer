# planwright Plan — .
<!-- Session: 2026-06-05T03:00:00Z -->

## Cycle 15 — Coverage + Opportunity (depth 10)

Audit found: `getChildProcesses()` and `getAllDescendantProcesses()` are only tested in the
"has children" case — the leaf-node empty-result path is untested. `getSystemCpuStats()` is
documented in `api-reference.md:67`, the `SystemCpuStats` struct is fully defined in
`system_model.h:30-41`, but the implementation is entirely absent. `api-reference.md` lacks
entries for `getProcessCpuAffinity()` and `getSystemCpuStats()` (both added in this session),
leaving callers with no docs for two public API methods.

---

### Item 1 — Leaf-node hierarchy empty results
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/process_details.cpp`  
**Surfaces:** `src/analyzer/base.cpp:154-179` (`getChildProcesses`, `getAllDescendantProcesses`)  
**Description:** Both functions are tested only when the target process has descendants.
The leaf-node case (process with no children) is untested — both should return an empty
vector on success. Add two tests using the existing `kGrand` process (which has no children
in the fixture):
- `TEST_F(ProcessDetailsTest, GetChildProcessesReturnsEmptyForLeafProcess)` — asserts
  `getChildProcesses(kGrand)` succeeds and returns empty.
- `TEST_F(ProcessDetailsTest, GetAllDescendantProcessesReturnsEmptyForLeafProcess)` — asserts
  `getAllDescendantProcesses(kGrand)` succeeds and returns empty.  
**Verification:** Build + ctest passes. Both new tests pass.  
**Status:** [x] done

---

### Item 2 — `getSystemCpuStats()` implementation
**Mode:** develop  
**Rung:** 3 (opportunity)  
**File:** `include/analyzer/core.h`, `src/analyzer/system.cpp`,
  `tests/analyzer/system_info.cpp`  
**Surfaces:** `docs/api-reference.md:67` (`getSystemCpuStats()` documented),
  `include/analyzer/system_model.h:30-41` (`SystemCpuStats` struct fully defined),
  `src/analyzer/system.cpp` (natural home: already implements other `/proc/stat` readers)  
**Description:** `api-reference.md` documents `getSystemCpuStats()` but the method is absent
from `core.h`. Implement it: read `/proc/stat`, find the `cpu ` aggregate line, parse the
10 fields into `SystemCpuStats` (user, nice, system, idle, iowait, irq, softirq, steal,
guest, guestNice). Return `analyzerParsingError` if the `cpu` line is absent. Add the
declaration to `core.h` under `//- System-wide Information & Statistics`. Implement in
`src/analyzer/system.cpp`. Add two tests to `tests/analyzer/system_info.cpp`:
- `TEST(SystemCpuStatsTest, ParsesCpuLine)` — create a mock `stat` file with a known `cpu`
  line, assert all 10 fields parse correctly.
- `TEST(SystemCpuStatsTest, MissingStatReturnsError)` — absent `stat` file returns error.  
**Verification:** Build + ctest passes. New tests cover the new API.  
**Status:** [x] done

---

### Item 3 — `api-reference.md` documents new APIs
**Mode:** docs  
**Rung:** 3 (opportunity)  
**File:** `docs/api-reference.md`  
**Surfaces:** `include/analyzer/core.h:75-76` (`getProcessCpuAffinity`,
  `setProcessCpuAffinity`), `include/analyzer/core.h` (`getSystemCpuStats` once added)  
**Description:** Two API methods added in Cycles 13–14 (`getProcessCpuAffinity`,
`setProcessCpuAffinity`) are missing from `api-reference.md`; the doc still uses the old
name `setProcessNiceness` instead of the actual method name `setProcessPriority`. Fix three
documentation gaps:
(a) Under "Process Control & Manipulation", add `getProcessCpuAffinity(pid)` before
`setProcessCpuAffinity`, and rename `setProcessNiceness` to `setProcessPriority`.
(b) Under "System-wide Information & Statistics", add `getSystemCpuStats()` once Item 2 is
done.  
**Verification:** `docs/api-reference.md` matches the actual `core.h` public surface for the
listed functions.  
**Status:** [x] done

