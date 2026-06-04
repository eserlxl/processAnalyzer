# planwright Plan — .
<!-- Session: 2026-06-05T03:00:00Z -->

## Cycle 10 — Repair + Coverage (depth 10)

Audit found: `--network <port>` list-filter documented but never wired in args/main;
`getDefaultColumnsForTable` has zero test coverage; `streamQueryProcesses` network-filter
branch in `base.cpp:214-225` is dead code in tests; `getSystemLoadAverage` malformed-content
error path is untested.

---

### Item 1 — Implement `--network <port>` list filter
**Mode:** repair  
**Rung:** 1  
**File:** `include/cli/args.h`, `src/cli/args.cpp`, `src/main.cpp`, `tests/cli/args.cpp`  
**Surfaces:** `docs/usage.md:136` (`--network <port>` filter entry); `include/cli/args.h:30`
(only `showNetworkConnections` bool, no port field); `src/cli/args.cpp:282-283` (parses
`--network` as no-arg flag, sets bool only); `src/main.cpp:32-46` (filter-population block,
no networkConnectionFilter wiring); `include/analyzer/process_model.h:131-139`
(`networkConnectionFilter.localPort` exists but never set by CLI)  
**Description:** `docs/usage.md` documents `--network <port>` as a `list` command filter
("Show only processes with an active connection on the given local port"), but:
(a) `ParsedArguments` has no field for a port filter — only `showNetworkConnections: bool`
for the `show` command; (b) `args.cpp` parses `--network` with no argument and sets the
bool; (c) `main.cpp` never sets `filter.networkConnectionFilter`; (d) the validation block
at `args.cpp:431-435` rejects `--network` unless the command is `show`/`pid`, making the
documented list-filter usage produce an error.
Fix: add `std::optional<uint16_t> networkPortFilter` to `ParsedArguments`; in
`args.cpp:282-283`, when `--network` is encountered, peek at `cliArgs[i+1]` — if it parses
as a valid port (1–65535), consume it and set `networkPortFilter`; otherwise set
`showNetworkConnections = true`; update the validation to allow `--network <port>` with
`list`; in `main.cpp` filter-population block, if `args.networkPortFilter` is set,
populate `filter.networkConnectionFilter = ProcessFilter::NetworkFilterCriteria{}` with
`localPort = *args.networkPortFilter`. Add one test in `tests/cli/args.cpp`:
`NetworkPortFilterForList` verifying `--network 8080` with `list` command sets
`networkPortFilter = 8080` and `showNetworkConnections = false`.  
**Verification:** Build + `ctest --test-dir build -j$(nproc)` passes; new test passes;
`--network 8080` with `list` is no longer rejected; `--network` without arg with `show`
still works.  
**Status:** [x] done

---

### Item 2 — `getDefaultColumnsForTable` test coverage
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/cli/output.cpp`  
**Surfaces:** `src/cli/output.cpp:76-83` (`getDefaultColumnsForTable` function)  
**Description:** `getDefaultColumnsForTable(bool fullDetails)` returns a different column
list depending on `fullDetails`. `fullDetails=true` → `{"pid","user","name","state","rss","vm","threads","cmdline"}`;
`fullDetails=false` → `{"pid","user","name","state","rss"}`. Neither path has a test.
Add two tests: `DefaultColumnsFullDetails` verifying the 8-element list, and
`DefaultColumnsBrief` verifying the 5-element list.  
**Verification:** Build + ctest passes. Both new tests pass and assert the exact column names.  
**Status:** [x] done

---

### Item 3 — `streamQueryProcesses` network filter path coverage
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/stream_query.cpp`  
**Surfaces:** `src/analyzer/base.cpp:214-225` (network-filter branch inside
`streamQueryProcesses` lazy path — calls `getNetworkConnections` per process and skips
if `passesNetworkFilter` fails)  
**Description:** `streamQueryProcesses` has two code paths: (a) sorted queries fall back to
`queryProcesses` (already tested in `query.cpp`), and (b) filter-only (no sort) streaming
that evaluates `networkConnectionFilter` inline. Path (b)'s network filter branch has zero
test coverage — the existing `StreamQueryProcessesTest` only exercises static filters
(name-contains). Add a new fixture `StreamQueryNetworkFilterTest` with one process that has
a TCP connection on port 8080 and one without, then add test
`StreamNetworkFilterKeepsMatchingProcess`: filter by `localPort=8080`, assert only the
matching process is yielded.  
**Verification:** Build + ctest passes. New test exercises `base.cpp:214-225` network
filter path.  
**Status:** [x] done

---

### Item 4 — `getSystemLoadAverage` malformed-content error path
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/system_load_average.cpp`  
**Surfaces:** `src/analyzer/system.cpp:106-108` (returns `analyzerParsingError` when
`iss >> avg.oneMin >> avg.fiveMin >> avg.fifteenMin` fails)  
**Description:** `getSystemLoadAverage` returns an error when `/proc/loadavg` exists but
its content cannot be parsed as three doubles. The `ParsesLoadAverageFields` test covers
the happy path and `MissingLoadavgReturnsError` covers the missing-file path. The malformed
branch at `system.cpp:107` is never exercised. Add
`GetSystemLoadAverageTest.MalformedLoadavgReturnsError` providing non-numeric content
(`"not a number\n"`) and asserting the result has no value.  
**Verification:** Build + ctest passes. New test covers the parsing-error branch.  
**Status:** [x] done

