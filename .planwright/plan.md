# planwright Plan — .
<!-- Session: 2026-06-05T03:00:00Z -->

## Cycle 9 — Coverage + Repair (depth 10)

Audit found: MockProc parallel race condition; docs lie about `--sort-by` valid values;
CLI args tests missing for 8 sort-by strings; integration tests missing for uidFilter,
userFilter, sort-by-uid, sort-by-user, sort-by-startTime; remoteAddressRegex untested.

---

### Item 1 — Fix MockProc parallel test isolation
**Mode:** repair  
**Rung:** 1  
**File:** `tests/utils/testing_framework.cpp`, `tests/utils/testing_framework.h`  
**Surfaces:** `tests/utils/testing_framework.cpp:40` (MockProc constructor)  
**Description:** When ctest runs with `-j>1`, multiple test binary instances create MockProc
directories with the same hard-coded names (e.g. "mock_proc_query_net_filter") under CWD,
causing concurrent create/delete races and flaky failures. Fix: prefix the root path with
the process PID so each binary instance gets an isolated namespace:
`fs::temp_directory_path() / ("pa_" + std::to_string(getpid())) / basePath`.
Add `<unistd.h>` include for `getpid()`.  
**Verification:** `ctest --test-dir build -j$(nproc)` passes 100% consistently across 3 runs.  
**Status:** [x] done

---

### Item 2 — Fix --sort-by documentation inconsistencies
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `docs/usage.md`  
**Surfaces:** `docs/usage.md:144,238` (--sort-by option table + example using "cpu")  
**Description:** The docs table lists `cpu`, `mem`, `elapsed-time`, `nice` as valid sort fields
but none exist in `stringToProcessSortField` in `src/cli/args.cpp`. The example at line 238
uses `--sort-by cpu` which returns an error at runtime. The docs also omit valid fields
`io-read`, `io-write`, `cpu-user-time`, `cpu-kernel-time`, `priority`.
Fix: replace the invalid field names with the real ones; update the example.  
**Verification:** Build + all tests pass. Cross-check `stringToProcessSortField` in args.cpp
matches the docs table exactly.  
**Status:** [x] done

---

### Item 3 — CLI args tests for untested sort-by strings
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/cli/args.cpp`  
**Surfaces:** `src/cli/args.cpp:26-44` (`stringToProcessSortField` branches for uid, user, rss, vm,
state, ppid, threads, start-time)  
**Description:** The `stringToProcessSortField` function handles 18 string-to-enum mappings. CLI
args tests currently cover only: pid, name, cmdline, exec-path, cwd, cpu-time, cpu-user-time,
cpu-kernel-time, io-read, io-write, priority (11 of 18). Missing tests for: uid, user, rss, vm,
state, ppid, threads, start-time. Add one test per missing string.  
**Verification:** Build + ctest passes. New tests verify each string maps to the correct
`ProcessSortField` enum value.  
**Status:** [x] done

---

### Item 4 — Integration tests for uidFilter and userFilter
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/query.cpp`  
**Surfaces:** `src/analyzer/internal/filter_helpers.cpp:36-38` (uidFilter, userFilter paths in
`passesStaticFilters`)  
**Description:** `ProcessFilter::uidFilter` and `ProcessFilter::userFilter` are implemented in
`filter_helpers.cpp` but have zero integration test coverage. The uidFilter path compares
`pInfo.uid != *filter.uidFilter`. Add a new fixture `QueryUidUserFilterTest` with two processes
having different UIDs (set via `withStatusField("Uid", "...")`) and tests for:
`FilterByUidFilterMatches`, `FilterByUidFilterNoMatch`.
For userFilter: since getpwuid depends on the system, test only that it filters by username
string match/no-match using the uid fallback ("100" vs "200").  
**Verification:** Build + ctest passes. New tests verify uid/user filter paths.  
**Status:** [x] done

---

### Item 5 — Sort-by-uid and sort-by-startTime ordering tests
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/query.cpp`  
**Surfaces:** `src/analyzer/query.cpp:60,55` (ProcessSortField::uid, ::startTime comparators)  
**Description:** `ProcessSortField::uid` and `ProcessSortField::startTime` comparators are live but
have no ordering test. For uid: use `QueryUidUserFilterTest` fixture (Item 4), asserting ascending
uid order. For startTime: create a fixture with two processes having different `starttime` values in
`ProcStatData` (field exists: `unsigned long long starttime = 0`), asserting ascending
startTimeUnix order.  
**Verification:** Build + ctest passes. New tests: `SortByUidAscending`, `SortByStartTimeAscending`.  
**Status:** [x] done

---

### Item 6 — remoteAddressRegex network filter test
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/query.cpp`  
**Surfaces:** `src/analyzer/internal/filter_helpers.cpp:57-59` (`remoteAddressRegex` path in
`passesNetworkFilter`)  
**Description:** `NetworkFilterCriteria::remoteAddressRegex` is implemented in filter_helpers.cpp
but has zero tests. `QueryNetworkRemoteFilterTest` already has a process with remoteAddress
"10.2.3.4". Add two tests: `FilterByRemoteAddressRegexMatches` (regex `"^10\\.2"` → kEstabPid),
`FilterByRemoteAddressRegexNoMatch` (regex `"^192\\."` → empty).  
**Verification:** Build + ctest passes. New tests cover the remoteAddressRegex branch.  
**Status:** [x] done

---

### Item 7 — Sort-by-user ordering test
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/query.cpp`  
**Surfaces:** `src/analyzer/query.cpp:61` (ProcessSortField::user comparator)  
**Description:** `ProcessSortField::user` comparator is untested for ordering. The username is
resolved via `getpwuid` or falls back to the numeric uid string. Use the `QueryUidUserFilterTest`
fixture's two processes (uid 100 → username "100", uid 200 → username "200") and add
`SortByUserAscending` test verifying "100" comes before "200" in ascending order.  
**Verification:** Build + ctest passes. New test: `SortByUserAscending`.  
**Status:** [x] done

---

### Item 8 — Opportunity: make sort-by fields documentation complete
**Mode:** docs  
**Rung:** 3 (opportunity)  
**File:** `docs/usage.md`  
**Surfaces:** `docs/usage.md:143-145`, `src/cli/args.cpp:23-44`  
**Description:** After Item 2 fixes the wrong sort-by fields, this item adds the Column Reference
table for the `--sort-by` option (currently the docs don't have a separate column reference for
sort fields), adding one-line descriptions for each valid sort-by key (io-read, io-write,
cpu-user-time, cpu-kernel-time, priority, start-time, ppid, uid, user, state, cwd, cmdline,
exec-path, vm, rss, threads, cpu-time). Clarify how `--sort-by` relates to the `--columns` list.  
**Verification:** No build changes; verify docs table has all 18 sort-by keys that match
`stringToProcessSortField`.  
**Status:** [x] done
