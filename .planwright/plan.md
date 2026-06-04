# planwright Plan — .
<!-- Session: 2026-06-05T00:00:00Z -->

## 1. [repair] Fix thread CPU-tick stat field offset in `getProcessThreads`

**Surfaces:** `src/analyzer/details.cpp:25,338`

**Evidence:** `constexpr int threadStatSkipFields = 14` at line 25. After reading `state` (field 3 of `/proc/pid/task/tid/stat`), fields 4–13 (ppid through cmajflt) = 10 fields precede `utime` (field 14). Skipping 14 lands on field 17 (`cstime`), so the read pair is `priority` (18) and `nice` (19), not `utime`/`stime`. Thread CPU times always return priority/nice values instead of actual ticks.

**Development:** Change `constexpr int threadStatSkipFields = 14;` to `constexpr int threadStatSkipFields = 10;`. Add a one-line comment enumerating the skipped fields (ppid, pgrp, session, tty_nr, tpgid, flags, minflt, cminflt, majflt, cmajflt).

**Verification:** Build succeeds. New regression test (Item 2) fails before this change and passes after. Existing `GetProcessThreadsTest` suite still passes.

---

## 2. [improve] Add thread CPU-tick regression test to `GetProcessThreadsTest`

**Surfaces:** `tests/analyzer/details.cpp` (GetProcessThreadsTest fixture)

**Evidence:** No existing test in `GetProcessThreadsTest` asserts `cpuUserTimeTicks` or `cpuKernelTimeTicks`. The Item 1 bug is invisible to the test suite — it can be re-introduced silently.

**Development:** In the existing mock proc setup for `GetProcessThreadsTest`, write a `/proc/<pid>/task/<tid>/stat` file with specific known values for fields 14 (`utime`) and 15 (`stime`). Assert that `ThreadInfo::cpuUserTimeTicks` equals the written utime and `cpuKernelTimeTicks` equals the written stime. Use named `constexpr` constants (`kUtime`, `kStime`). This test must fail before Item 1 is applied.

**Verification:** Test fails on the unpatched codebase (`threadStatSkipFields = 14`), passes after the fix (`= 10`). Build and full test suite pass.

---

## 3. [improve] Reject negative UIDs at parse time; align `uidFilter` types across the CLI–analyzer boundary

**Surfaces:** `src/cli/args.cpp` (`--uid` parsing block), `include/cli/args.h` (`ParsedArguments::uidFilter`), `src/main.cpp` (filter population)

**Evidence:** `ParsedArguments::uidFilter` is `std::optional<int>`; `ProcessFilter::uidFilter` is `std::optional<uint32_t>`. The assignment `filter.uidFilter = args.uidFilter` at the layer join compiles without warning (implicit narrowing). Passing `--uid -1` silently produces a filter for UID 4294967295, matching no process and producing a confusing zero-result instead of an error.

**Development:** In the `--uid` parsing block in `src/cli/args.cpp`, after calling `parseIntWithinRange`, add a check: if the parsed value is `< 0`, print `"Error: --uid requires a non-negative integer.\n"` and return `std::nullopt`. No type change needed in `ParsedArguments` — the runtime guard at the boundary is sufficient and avoids cascading changes.

**Verification:** `cmake --build build && ctest --test-dir build` passes. New tests: `UidFilterNegativeValueReturnsNullopt` (passes `-1`, expects nullopt). Existing UID tests unchanged.

---

## 4. [improve] Test `streamQueryProcesses` with an active `networkConnectionFilter`

**Surfaces:** `tests/analyzer/query.cpp` (new or extended fixture)

**Evidence:** `QueryNetworkFilterTest` (added in Cycle 4) only calls `queryProcesses`. `streamQueryProcesses` applies `Internal::passesNetworkFilter` in its own coroutine loop — this branch is exercised by no test, so a regression there would be invisible.

**Development:** In `tests/analyzer/query.cpp`, add a test `StreamQueryAppliesNetworkFilter` that calls `streamQueryProcesses` with a `ProcessFilter` whose `networkConnectionFilter.localPort = kListeningPort`. Iterate the generator and assert only the mock process with that port appears (reuse the `QueryNetworkFilterTest` mock setup via a shared helper or a new fixture). Also add a negative case: filter for `kNonListeningPort`, assert the generator yields no results.

**Verification:** Build and full test suite (`ctest`) pass. Coverage of `streamQueryProcesses`'s network-filter branch is now exercised.

---

## 5. [docs] Update `docs/usage.md` to cover all post-Cycle-3 features

**Surfaces:** `docs/usage.md`

**Evidence:** The file is 240 lines describing the pre-Cycle-3 state. Missing entirely: the `system` command and its subsections (System Information, Load Average, Memory, Disk Usage, Network Interfaces, Disk I/O Stats, System Activity), `--uid`, `--min-rss`, `--max-rss`, `--min-threads`, `--max-threads`, `cwd` column, CPU User/Kernel Time and IO Read/Write in vertical output, and the extended `--sort-by` fields (`cmdline`, `cwd`, `cpu-time`, `elapsed-time`, `exec-path`, `nice`).

**Development:** Add a `## system command` section with a description and example output block. Extend the filter flags table with `--uid`, `--min-rss`, `--max-rss`, `--min-threads`, `--max-threads`. Add `cwd` to the columns reference table. Extend the `--sort-by` values list. Add the CPU/IO fields to the vertical output description.

**Verification:** Grep the file for each added keyword (`--uid`, `system`, `cwd`, `CPU User`, etc.) — all present. No broken cross-references. Build (`cmake --build build`) and tests still pass.

---

## 6. [develop] Add `--cmdline <pattern>` CLI filter flag

**Surfaces:** `include/cli/args.h` (`ParsedArguments`), `src/cli/args.cpp` (parsing + usage), `src/main.cpp` (filter wiring)

**Evidence:** `ProcessFilter::cmdlineContains` is implemented and applied in `passesStaticFilters` (filter_helpers.cpp:34) but has no CLI flag. Users cannot filter by command-line substring without writing code.

**Development:** Add `std::optional<std::string> cmdlineFilter;` to `ParsedArguments` in `args.h`. In `args.cpp`, add a `--cmdline <pattern>` block (analogous to `--name`): require the next token, store it. Add usage line. In `main.cpp`, wire: `if (args.cmdlineFilter) filter.cmdlineContains = *args.cmdlineFilter;`. Add a test `CmdlineFilterIsSet` in `tests/cli/args.cpp`.

**Verification:** Build passes. `CmdlineFilterIsSet` test passes. Manual smoke: `processAnalyzer list --cmdline bash` returns only processes whose cmdline contains "bash".

---

## 7. [develop] Add `--min-vm <KB>` / `--max-vm <KB>` CLI filter flags

**Surfaces:** `include/cli/args.h`, `src/cli/args.cpp`, `src/main.cpp`

**Evidence:** `ProcessFilter::minVirtualMemoryKB` and `maxVirtualMemoryKB` are fully implemented (process_model.h:117–118, filter_helpers.cpp:41) but unreachable from the CLI.

**Development:** Add `std::optional<long long> minVmKb;` and `std::optional<long long> maxVmKb;` to `ParsedArguments`. In `args.cpp`, add `--min-vm <KB>` and `--max-vm <KB>` parsing blocks using the existing `parseIntWithinRange` pattern (reject negative, cast to `long long`). Wire in `main.cpp`. Add usage lines. Add tests `MinVmFilterIsSet` and `MaxVmFilterIsSet` in `tests/cli/args.cpp`.

**Verification:** Build and `ctest` pass. New arg tests pass. Cross-check: `--min-vm 0 --max-vm 0` returns no processes (no process has 0 KB virtual memory).

---

## 8. [develop] Add `--min-priority <N>` / `--max-priority <N>` CLI filter flags

**Surfaces:** `include/cli/args.h`, `src/cli/args.cpp`, `src/main.cpp`

**Evidence:** `ProcessFilter::minPriority` and `maxPriority` (`optional<int>`) are fully implemented (process_model.h:126–127, filter_helpers.cpp:42) but unreachable from the CLI. Priority is a signed integer in the kernel (negative = high priority for RT processes).

**Development:** Add `std::optional<int> minPriority;` and `std::optional<int> maxPriority;` to `ParsedArguments`. Add `--min-priority <N>` and `--max-priority <N>` parsing blocks in `args.cpp` using `parseIntWithinRange` (signed, so negative values are valid). Wire in `main.cpp`. Add usage lines. Add tests `MinPriorityFilterIsSet` and `MaxPriorityFilterIsSet` in `tests/cli/args.cpp`.

**Verification:** Build and `ctest` pass. New arg tests pass. Existing `QueryFiltersTest` suite still passes.
