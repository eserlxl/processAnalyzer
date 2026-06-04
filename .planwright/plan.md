# planwright Plan — .
<!-- Session: 2026-06-05T03:00:00Z -->

## Cycle 8 — Coverage (depth 10)

Audit found 16 of 18 ProcessSortField enum values with zero test coverage, plus three untested
filter paths (cmdlineRegex, executablePathRegex, remotePort/remoteAddressContains).
All items are rung-2 coverage unless noted.

---

### Item 1 — Sort by RSS and VmSize
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/query.cpp`  
**Surfaces:** `src/analyzer/query.cpp:53-54` (ProcessSortField::rss, ::vmsize comparators)  
**Description:** Add `QuerySortByStatTest` fixture with two processes having distinct `rss` and
`vsize` values in `ProcStatData`. Tests verify ascending order for `ProcessSortField::rss` and
`ProcessSortField::vmsize`. Existing `QueryStatFilterTest` only tests filtering, not sort ordering
on these fields.  
**Verification:** Build + ctest passes. New tests: `SortByRssAscending`, `SortByVmsizeAscending`.  
**Status:** [x] done

---

### Item 2 — Sort by threads and priority
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/query.cpp`  
**Surfaces:** `src/analyzer/query.cpp:63,69` (ProcessSortField::threads, ::priority comparators)  
**Description:** Extend `QuerySortByStatTest` with processes differing in `num_threads` and
`priority`. Tests verify ascending order for `ProcessSortField::threads` and
`ProcessSortField::priority`. Both comparator branches are live but untested.  
**Verification:** Build + ctest passes. New tests: `SortByThreadsAscending`, `SortByPriorityAscending`.  
**Status:** [x] done

---

### Item 3 — Sort by CPU ticks (cpuUserTime, cpuKernelTime, cpuTime)
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/query.cpp`  
**Surfaces:** `src/analyzer/query.cpp:56,65-66` (::cpuTime, ::cpuUserTime, ::cpuKernelTime)  
**Description:** Extend `QuerySortByStatTest` with `utime` and `stime` differences in `ProcStatData`.
Tests verify ordering for all three CPU-tick sort fields: `cpuUserTime`, `cpuKernelTime`, and
the combined `cpuTime` (utime+stime). The threadStatSkipFields fix in Cycle 5 ensures the right
values are populated; this closes the coverage gap on those comparator branches.  
**Verification:** Build + ctest passes. New tests: `SortByCpuUserTimeAscending`, `SortByCpuKernelTimeAscending`, `SortByCpuTimeAscending`.  
**Status:** [x] done

---

### Item 4 — Sort by ioReadBytes and ioWriteBytes
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/query.cpp`  
**Surfaces:** `src/analyzer/query.cpp:67-68` (::ioReadBytes, ::ioWriteBytes comparators)  
**Description:** Extend `QuerySortByStatTest` with `ProcIoStats::readBytes` and `writeBytes`
differences, using `withIoStats`. Tests verify ascending ordering for both I/O byte sort fields.
These comparator branches are reachable only when processes have been built with io stat data.  
**Verification:** Build + ctest passes. New tests: `SortByIoReadBytesAscending`, `SortByIoWriteBytesAscending`.  
**Status:** [x] done

---

### Item 5 — Sort by state and ppid
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/query.cpp`  
**Surfaces:** `src/analyzer/query.cpp:52,62` (::ppid, ::state comparators)  
**Description:** Add `QuerySortByStateAndPpidTest` fixture with two processes: one with state='R'
and ppid=1, another with state='S' and ppid=2. Tests verify `ProcessSortField::state` and
`ProcessSortField::ppid` ascending ordering. State and ppid comparators exist but have no tests.  
**Verification:** Build + ctest passes. New tests: `SortByStateAscending`, `SortByPpidAscending`.  
**Status:** [x] done

---

### Item 6 — Sort by executablePath, cmdline, cwd
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/query.cpp`  
**Surfaces:** `src/analyzer/query.cpp:58-59,64` (::executablePath, ::cmdline, ::cwd comparators)  
**Description:** Add `QuerySortByPathTest` fixture with two processes differing in executable path
(`withExe`), cmdline (`withCmdline`), and cwd (`withCwd`). Tests verify ascending ordering for
`ProcessSortField::executablePath`, `::cmdline`, and `::cwd`. All three comparator branches are
untested despite being on live code paths.  
**Verification:** Build + ctest passes. New tests: `SortByExecPathAscending`, `SortByCmdlineAscending`, `SortByCwdAscending`.  
**Status:** [x] done

---

### Item 7 — cmdlineRegex and executablePathRegex filter tests
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/query.cpp`  
**Surfaces:** `src/analyzer/internal/filter_helpers.cpp:34-35` (`cmdlineRegex`, `executablePathRegex` paths in `applyStringFilter`)  
**Description:** Add regex filter tests using `ProcessFilter::cmdlineRegex` and
`executablePathRegex` with `std::regex`. The `applyStringFilter` function handles both `contains`
and regex paths, but the regex branches for cmdline and executablePath are exercised by zero tests.
Tests: match case (regex matches subset), no-match case (regex matches nothing).  
**Verification:** Build + ctest passes. New tests: `FilterByCmdlineRegexMatchesSubset`,
`FilterByCmdlineRegexNoMatch`, `FilterByExecPathRegexMatchesSubset`, `FilterByExecPathRegexNoMatch`.  
**Status:** [x] done

---

### Item 8 — remotePort and remoteAddressContains network filter tests
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/query.cpp`  
**Surfaces:** `src/analyzer/internal/filter_helpers.cpp:53,56` (`remotePort`, `remoteAddressContains` in `passesNetworkFilter`)  
**Description:** Extend `QueryNetworkFilterTest` to include a mock `net/tcp` entry with a
non-zero remote port (e.g. 443) and remote address (e.g. `1.2.3.4`). Tests verify:
`FilterByRemotePortMatches` (port 443 → returns netproc), `FilterByRemotePortNoMatch` (port 80 →
empty), `FilterByRemoteAddressContainsMatches` ("1.2.3" → returns netproc). These filter branches
exist in `passesNetworkFilter` but have zero test coverage.  
**Verification:** Build + ctest passes. New tests: `FilterByRemotePortMatches`, `FilterByRemotePortNoMatch`, `FilterByRemoteAddressContainsMatches`.  
**Status:** [x] done
