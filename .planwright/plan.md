# planwright Plan — .
<!-- Session: 2026-06-05T03:00:00Z -->

## Cycle 12 — Coverage (depth 10)

Audit found: `setCurrentWorkingDirectory` is untested (empty-path error + success);
TCP6 (`net/tcp6`) network connection parsing has zero test coverage;
`executeCommand` stderr-capture path is untested; `--network <port>` with the `show`
command (validation rejection) is untested.

---

### Item 1 — `setCurrentWorkingDirectory` test coverage
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/utils/system.cpp`  
**Surfaces:** `src/utils/system.cpp:170-182` (`setCurrentWorkingDirectory`)  
**Description:** `setCurrentWorkingDirectory` has two testable branches: (a) empty path →
`invalidArgument`; (b) valid existing directory → changes the CWD. Both are untested.
Add `TEST(SystemUtilsTest, SetCurrentWorkingDirectoryRejectsEmptyPath)` asserting
`invalidArgument` error, and `TEST(SystemUtilsTest, SetCurrentWorkingDirectoryChangesDir)`
which saves the original CWD (`getCurrentWorkingDirectory`), changes to a temp dir, checks
the new CWD, then restores.  
**Verification:** Build + ctest passes. Both new tests pass.  
**Status:** [x] done

---

### Item 2 — TCP6 network connection parsing test
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/network_connections.cpp`  
**Surfaces:** `src/analyzer/network.cpp:224` (`processConnection(netPath / "tcp6", "TCP6")`)  
**Description:** `getNetworkConnections` reads `net/tcp6` and parses IPv6 connections, but
no test exercises this path. The existing suite tests only TCP (IPv4) and UDP. Add a
standalone test `GetNetworkConnectionsTcp6.ParsesTcp6Connection`: set up a process with a
socket fd whose inode appears in `net/tcp6` (loopback `::1` → hex
`00000000000000000000000001000000:2328`), then assert that `getNetworkConnections` returns
one connection with `protocol == "TCP6"` and `localPort == 9000` (0x2328).  
**Verification:** Build + ctest passes. New test covers the TCP6 branch.  
**Status:** [x] done

---

### Item 3 — `executeCommand` stderr capture test
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/utils/system.cpp`  
**Surfaces:** `src/utils/system.cpp:103-117` (stderr temp-file read-back path inside
`executeCommand`)  
**Description:** The existing `ExecuteCommandCapturesStdout` test only exercises the stdout
path. The stderr-capture path (reads the temp file written by `2>`) is not covered. Add
`TEST(SystemUtilsTest, ExecuteCommandCapturesStderr)` running `echo error >&2` (or a
command that writes to stderr), asserting `result.value().stderrStr` contains `"error"`.  
**Verification:** Build + ctest passes. New test covers the stderr path.  
**Status:** [x] done

---

### Item 4 — `--network <port>` validation with show command
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/cli/args.cpp`  
**Surfaces:** `src/cli/args.cpp` (validation block: `--network <port>` is only valid with
`list`; rejected for `show`/`pid`)  
**Description:** The new validation added in Cycle 10 rejects `--network <port>` when the
command is `show` or `pid` (emitting "only valid with 'list' command"). This path has no
test. Add `ArgsTestFixture.NetworkPortFilterRejectedForShowCommand`:
`processAnalyzer show --pid 1 --network 8080` should return `std::nullopt` because the
port filter is not valid with `show`.  
**Verification:** Build + ctest passes. New test confirms the validation error fires.  
**Status:** [x] done

