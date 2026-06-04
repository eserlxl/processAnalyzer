- [x] Fix strict-weak-ordering violation in descending process sort
      Mode: repair
      Rationale: The descending-sort branch returns the negation of the ascending comparison, so equal elements compare true in both directions, violating the strict-weak-ordering precondition of std::ranges::sort (undefined behavior), and queryProcesses has no test to catch it.
      Evidence: src/analyzer/query.cpp:143 returns `(sortOrder == SortOrder::asc) ? less : !less;` where `less` is `a.field < b.field`; for SortOrder::desc and two elements with an equal key, comp(a,b)==comp(b,a)==true, which std::ranges::sort (src/analyzer/query.cpp:118) forbids.
      Surfaces: src/analyzer/query.cpp
      New Surfaces: tests/analyzer/query.cpp
      Development: In queryProcesses (src/analyzer/query.cpp:118-145), factor the per-field key comparison into a `compareLess(const ProcessInfo&, const ProcessInfo&)` lambda and return `(sortOrder == SortOrder::asc) ? compareLess(a, b) : compareLess(b, a);` so descending swaps operands instead of negating. Create tests/analyzer/query.cpp with a GTest fixture QueryProcessesTest mirroring GetNetworkInterfaceStatsTest (tests/analyzer/network_interface_stats.cpp): point ProcessAnalyzer at a MockProc and use mockProc->buildProcess(pid).withName(...).withParent(...).withStat(...) (ProcessBuilder, tests/utils/testing_framework.h:191) to create several processes including two with an equal name but different pids; assert ascending sort by ProcessSortField::pid and descending sort by ProcessSortField::name both return correctly ordered results (the descending/equal-name case is the regression guard). tests/CMakeLists.txt GLOB_RECURSE CONFIGURE_DEPENDS picks the new file up automatically.
      Acceptance: Descending sort returns correctly ordered results with no strict-weak-ordering violation; the new QueryProcessesTest cases pass and all existing tests still pass.
      Verification: ctest --test-dir build -R "QueryProcessesTest" --output-on-failure

- [x] Fill empty network_connections test stub with getNetworkConnections coverage
      Mode: improve
      Rationale: tests/analyzer/network_connections.cpp is a compiled-but-empty file, leaving the public getNetworkConnections parser untested.
      Evidence: tests/analyzer/network_connections.cpp is 0 lines yet linked into ProcessAnalyzerTests via tests/CMakeLists.txt:7 GLOB_RECURSE; src/analyzer/network.cpp:186 defines ProcessAnalyzer::getNetworkConnections, which reads /proc/net/{tcp,tcp6,udp,udp6} and matches socket inodes from process fds.
      Surfaces: tests/analyzer/network_connections.cpp
      Development: Add a GetNetworkConnectionsTest fixture (same MockProc pattern as network_interface_stats.cpp): build a process whose fd targets a "socket:[<inode>]" via mockProc->buildProcess(pid).withFd(fd, "socket:[12345]"), create net/tcp with mockProc->createFileAt containing a header line plus a row whose inode column equals 12345 and a known hex local/remote address+port and TCP state (e.g. "0100007F:1F90"), then assert getNetworkConnections(pid) returns one NetworkConnection with the decoded localAddress/localPort/state. Add an empty-result case (no matching inode) returning an empty vector.
      Acceptance: GetNetworkConnectionsTest parses a mocked TCP connection and the no-match case; the previously empty file now contributes passing tests and existing tests still pass.
      Verification: ctest --test-dir build -R "GetNetworkConnectionsTest" --output-on-failure

- [x] Use std::string::contains for membership checks in query filters
      Mode: improve
      Rationale: Two substring-membership checks use the find()==npos idiom that clang-tidy readability-container-contains flags; std::string::contains (C++23, already used elsewhere in this codebase) is clearer and matches house style.
      Evidence: src/analyzer/query.cpp:19 `targetString.find(*containsFilter) == std::string::npos` and src/analyzer/query.cpp:103 `conn.remoteAddress.find(*netFilter.remoteAddressContains) == std::string::npos` both trigger clang-tidy readability-container-contains; src/utils/string.cpp:49 already uses s.contains(substring).
      Surfaces: src/analyzer/query.cpp
      Development: In applyStringFilter (src/analyzer/query.cpp:16) rewrite the containsFilter guard as `if (containsFilter && !targetString.contains(*containsFilter))`, and in queryProcesses' network-filter block (src/analyzer/query.cpp:103) rewrite as `if (netFilter.remoteAddressContains && !conn.remoteAddress.contains(*netFilter.remoteAddressContains)) currentConnMatch = false;`. Behavior is identical (negated membership).
      Acceptance: Both clang-tidy readability-container-contains warnings on src/analyzer/query.cpp are gone; substring filtering behavior is unchanged and QueryProcessesTest still passes.
      Verification: ctest --test-dir build -R "QueryProcessesTest" --output-on-failure

- [x] Extend queryProcesses coverage to range filters and descending pid sort
      Mode: improve
      Rationale: The new QueryProcessesTest covers name/state/ppid filters and name sorting, but the numeric range-filter helper and the descending branch for a numeric field remain unexercised.
      Evidence: src/analyzer/query.cpp:30 applyRangeFilter (used at query.cpp:74-77 for minThreads/maxThreads and memory) and the descending numeric path in the sort comparator (src/analyzer/query.cpp) have no assertions in tests/analyzer/query.cpp.
      Surfaces: tests/analyzer/query.cpp
      Development: Add QueryProcessesTest cases using the existing fixture (all four processes have threadCount 1): minThreads=1 returns all four, minThreads=2 returns an empty vector, maxThreads=1 returns all four (exercising both applyRangeFilter branches), and a SortDescendingByPid case asserting pids come back in {40,30,20,10} order (the descending numeric branch).
      Acceptance: New range-filter and descending-pid QueryProcessesTest cases pass alongside the existing ones; all tests still pass.
      Verification: ctest --test-dir build -R "QueryProcessesTest" --output-on-failure

- [x] Escape control characters in JSON export and add CLI output tests
      Mode: repair
      Rationale: printProcessJson escapes only backslash and double-quote, so any process string field containing a control character (tab, newline, etc.) is emitted literally, producing JSON that violates RFC 8259 and is rejected by strict parsers; the entire CLI output layer is also untested.
      Evidence: src/cli/output.cpp:158-160 only applies replaceAll for "\\" and "\"" before quoting string values, leaving control bytes (U+0000-U+001F) unescaped; a ProcessInfo whose name is "a\tb" yields `"name": "a<TAB>b"`, which is invalid JSON. tests/cli/output.cpp is 0 lines, so printProcessJson/printProcessCsv/printProcessTable have no tests.
      Surfaces: src/cli/output.cpp, tests/cli/output.cpp
      Development: In src/cli/output.cpp add a `jsonEscape(std::string_view)` helper in the anonymous namespace that emits \" \\ \b \f \n \r \t for those characters and \u00XX (lowercase hex) for any other byte < 0x20, then replace the two replaceAll lines in printProcessJson (src/cli/output.cpp:158-160) with a single jsonEscape call. Create tests/cli/output.cpp constructing ProcessInfo aggregates directly and using testing::internal::CaptureStdout()/GetCapturedStdout(): assert printProcessJson escapes tab/newline as \t/\n and quote/backslash as \" /\\, emits numeric columns (pid) unquoted, that printProcessCsv quotes a value containing a comma, and a printProcessTable smoke test prints an uppercased header. GLOB_RECURSE CONFIGURE_DEPENDS picks the new file up.
      Acceptance: JSON output escapes control characters into valid JSON escapes; new OutputTest cases pass and all existing tests still pass.
      Verification: ctest --test-dir build -R "OutputTest" --output-on-failure

- [x] Quote CSV fields containing newlines per RFC 4180
      Mode: improve
      Rationale: printProcessCsv only quotes fields containing a comma or double-quote, so a value with an embedded newline or carriage return splits one logical record across multiple physical CSV lines, corrupting the output.
      Evidence: src/cli/output.cpp quotes only when `value.contains(',') || value.contains('"')`; RFC 4180 §2 requires fields containing CR or LF to also be enclosed in double quotes, and process fields such as cmdline can contain embedded newlines.
      Surfaces: src/cli/output.cpp, tests/cli/output.cpp
      Development: In printProcessCsv (src/cli/output.cpp) extend the quoting predicate to `value.contains(',') || value.contains('"') || value.contains('\n') || value.contains('\r')`; the existing doubling of embedded quotes already makes a newline-bearing field a valid quoted field. Add an OutputTest case in tests/cli/output.cpp asserting that a cmdline value containing a newline is emitted wrapped in double quotes.
      Acceptance: CSV fields containing CR/LF are quoted; the new test passes and existing OutputTest cases still pass.
      Verification: ctest --test-dir build -R "OutputTest" --output-on-failure

- [x] Cover JSON array structure and vertical details in output tests
      Mode: improve
      Rationale: OutputTest covers single-object JSON and table/CSV basics but not the multi-process JSON array separators or printVerticalProcessDetails, both user-facing output paths.
      Evidence: printProcessJson (src/cli/output.cpp:142) wraps objects in `[` ... `]` and inserts a comma between objects, and printVerticalProcessDetails (src/cli/output.cpp:48) emits labelled fields; neither is asserted in tests/cli/output.cpp.
      Surfaces: tests/cli/output.cpp
      Development: Add an OutputTest case calling printProcessJson with two ProcessInfo objects and asserting the output starts with "[", ends with "]", and contains "},\n" between the two objects (the array separator), and a case calling printVerticalProcessDetails asserting it contains "PID:" and the process name.
      Acceptance: New JSON-array and vertical-details OutputTest cases pass alongside existing ones.
      Verification: ctest --test-dir build -R "OutputTest" --output-on-failure

- [x] Make streamPids tolerate an inaccessible /proc and add streaming tests
      Mode: improve
      Rationale: getPids guards its /proc directory iteration with try/catch and returns an error, but the README-highlighted streaming path streamPids iterates without any guard, so a filesystem error (e.g. procPath is not a directory or becomes unreadable) throws std::filesystem_error out of the std::generator to the caller; the streaming API also has no tests.
      Evidence: src/analyzer/base.cpp:54 `for (const auto& entry : fs::directory_iterator(procPath))` has no error handling, whereas getPids (src/analyzer/base.cpp:28-45) wraps the same iteration in try/catch; no test references streamPids or streamProcesses.
      Surfaces: src/analyzer/base.cpp
      New Surfaces: tests/analyzer/stream.cpp
      Development: In streamPids (src/analyzer/base.cpp:49) use the non-throwing std::error_code overloads: check `fs::exists(procPath, ec)` and co_return on error, construct `fs::directory_iterator(procPath, ec)` and co_return if ec is set, then iterate with a manual `while (it != end)` loop calling `entry.is_directory(dirEc)` and `it.increment(ec)` (co_return on increment error) so no filesystem_error can escape the coroutine; add `#include <system_error>`. Create tests/analyzer/stream.cpp: a MockProc-backed case asserting streamProcesses() yields the built processes (happy path), and a case that points the analyzer at a regular file (not a directory) and drains streamProcesses() into a vector, asserting it is empty and that draining does not throw.
      Acceptance: streamProcesses() over an invalid procPath returns no elements without throwing; the happy-path streaming test yields the expected pids; all existing tests still pass.
      Verification: ctest --test-dir build -R "StreamTest" --output-on-failure

- [x] Add direct coverage for getProcessDetails parsing and the process hierarchy
      Mode: improve
      Rationale: The core parser getProcessDetails and the hierarchy/snapshot API (getParentProcess, getChildProcesses, getAllDescendantProcesses, snapshot) have no direct tests, despite the README advertising 100% coverage; only the MockProc framework that produces the fixtures is tested, not the analyzer code that parses it.
      Evidence: No test outside tests/utils/testing_framework references getProcessDetails, getParentProcess, getChildProcesses, getAllDescendantProcesses, or snapshot (grep over tests/ finds zero); these are defined in src/analyzer/details.cpp:197 and src/analyzer/base.cpp:66-161.
      Surfaces: src/analyzer/details.cpp, src/analyzer/base.cpp
      New Surfaces: tests/analyzer/process_details.cpp
      Development: Create tests/analyzer/process_details.cpp with a ProcessDetailsTest MockProc fixture. For getProcessDetails: build a process via buildProcess(pid).withName(...).withParent(...).withCmdline(...).withExe(...).withStatusField("Uid","1000 1000 1000 1000").withStatusField("VmRSS","2048 kB").withStatusField("Threads","4") and assert info.pid, info.name, info.ppid, info.state, info.uid, info.residentMemory, info.threadCount, info.cmdline and info.executablePath are parsed; assert getProcessDetails on an absent pid returns an error (has_value() false). For the hierarchy: build a parent and two children (withParent(parentPid)) and assert getChildProcesses returns both child pids, getParentProcess(childPid) returns the parent, getParentProcess of a pid whose ppid is 1 returns an error, snapshot() returns all built processes, and getAllDescendantProcesses(parentPid) includes a grandchild built under one child. Assert on numeric uid (getUserName falls back to the numeric string, so do not assert a system username).
      Acceptance: New ProcessDetailsTest cases pass and exercise getProcessDetails field parsing plus the hierarchy/snapshot API; all existing tests still pass.
      Verification: ctest --test-dir build -R "ProcessDetailsTest" --output-on-failure

- [x] Add direct coverage for getProcessThreads and getProcessOpenFileDetails
      Mode: improve
      Rationale: Two core per-process inspection APIs the README advertises (threads, open files) have no direct tests; only their indirect use through getNetworkConnections exercises any of getProcessOpenFileDetails, and getProcessThreads is wholly untested.
      Evidence: No test references getProcessThreads or getProcessOpenFileDetails directly (grep over tests/ finds none); they are defined in src/analyzer/details.cpp:289 and src/analyzer/details.cpp:352, and classify fd targets by the socket:/pipe:/anon_inode:/ '/'-path prefixes.
      Surfaces: src/analyzer/details.cpp
      New Surfaces: tests/analyzer/threads_and_files.cpp
      Development: Create tests/analyzer/threads_and_files.cpp. GetProcessThreadsTest: build a process, add a second thread via mockProc->addThread(pid, tid, {.name="worker"}) (MockProc::AddThreadOptions, tests/utils/testing_framework.h:142,180), call getProcessThreads(pid) and assert it returns two ThreadInfo entries whose tids are {pid, tid} and that the worker thread name is parsed. GetOpenFilesTest: build a process with fds via buildProcess(pid).withFd(0,"socket:[12345]").withFd(1,"pipe:[67890]").withFd(2,"anon_inode:[eventfd]"), plus a real regular file created with mockProc->createFileAt("realfile","x") pointed to by withFd(3, absolute path to that file); call getProcessOpenFileDetails(pid) and assert the fds classify as OpenFileType::socket, pipe, anonInode and file respectively; assert getProcessOpenFileDetails on a pid with no fd dir returns an error.
      Acceptance: New GetProcessThreadsTest and GetOpenFilesTest cases pass and exercise thread enumeration and fd-type classification; all existing tests still pass.
      Verification: ctest --test-dir build -R "GetProcessThreadsTest|GetOpenFilesTest" --output-on-failure

- [x] Implement getProcessEnvironment (declared but missing)
      Mode: develop
      Rationale: getProcessEnvironment is declared in the public API and advertised in the README ("Environment variables"), but no implementation exists, so the symbol is undefined and the feature is unusable; the model, the /proc/<pid>/environ seam, and MockProc support all already exist.
      Evidence: include/analyzer/core.h:45 declares `utils::Result<std::vector<std::string>> getProcessEnvironment(int pid) const;` but no `ProcessAnalyzer::getProcessEnvironment` definition exists in src/ (grep finds zero; nm shows it absent from libprocessAnalyzerLib.a); MockProc exposes createEnviron/withEnviron (tests/utils/testing_framework.cpp:503) and parseCmdlineFile (src/analyzer/details.cpp:184) already demonstrates reading the null-separated /proc layout.
      Surfaces: src/analyzer/details.cpp
      New Surfaces: tests/analyzer/environment.cpp
      Development: Implement ProcessAnalyzer::getProcessEnvironment in src/analyzer/details.cpp following the getProcessOpenFileDetails pattern: call Internal::checkPidPathExistsAndPermissions(procPath, pid) and propagate its error, read (procPath / std::to_string(pid) / "environ") with utils::readTextFile, propagate its error, then split the null-separated content with utils::split(*content, '\0', /*skipEmpty=*/true) and return the resulting vector of "KEY=VALUE" entries. Create tests/analyzer/environment.cpp: build a process with buildProcess(pid).withEnviron({{"HOME","/root"},{"PATH","/usr/bin"}}), assert getProcessEnvironment(pid) contains "HOME=/root" and "PATH=/usr/bin", and assert an absent pid returns an error.
      Acceptance: getProcessEnvironment returns the process environment entries from /proc/<pid>/environ; the new GetProcessEnvironmentTest passes and all existing tests still pass.
      Verification: ctest --test-dir build -R "GetProcessEnvironmentTest" --output-on-failure

- [x] Implement getSystemMemoryInfo (declared but missing)
      Mode: develop
      Rationale: getSystemMemoryInfo is declared in the public API and the README advertises comprehensive memory statistics, but no implementation exists, so the symbol is undefined and the feature is unusable; the SystemMemoryInfo model and MockProc::createMeminfo already exist.
      Evidence: include/analyzer/core.h:56 declares `utils::Result<SystemMemoryInfo> getSystemMemoryInfo() const;` with no definition in src/ (grep finds zero; nm shows it absent from libprocessAnalyzerLib.a); SystemMemoryInfo (include/analyzer/system_model.h:14) has memTotal/memFree/memAvailable/buffers/cached/swapTotal/swapFree, and MockProc::createMeminfo writes the matching /proc/meminfo lines (tests/utils/testing_framework.cpp:230).
      Surfaces: src/analyzer/system.cpp
      New Surfaces: tests/analyzer/system_memory.cpp
      Development: Implement ProcessAnalyzer::getSystemMemoryInfo in src/analyzer/system.cpp: read (procPath / "meminfo") with utils::readTextFile and propagate its error, then for each line use a small lambda that does `std::istringstream ls(line); ls >> label >> value >> unit;` to fill the SystemMemoryInfo field whose meminfo label matches via line.starts_with ("MemTotal:", "MemFree:", "MemAvailable:", "Buffers:", "Cached:", "SwapTotal:", "SwapFree:" -- note "Cached:" does not match "SwapCached:"). Create tests/analyzer/system_memory.cpp: mockProc->createMeminfo with known kB values and assert getSystemMemoryInfo() returns them; assert that pointing the analyzer at a procPath without a meminfo file returns an error.
      Acceptance: getSystemMemoryInfo parses /proc/meminfo into SystemMemoryInfo; the new GetSystemMemoryInfoTest passes and all existing tests still pass.
      Verification: ctest --test-dir build -R "GetSystemMemoryInfoTest" --output-on-failure

- [x] Implement getSystemLoadAverage (declared but missing)
      Mode: develop
      Rationale: getSystemLoadAverage is declared in the public API and the README advertises load-average monitoring, but no implementation exists; the model and /proc/loadavg seam both exist.
      Evidence: include/analyzer/core.h:57 declares `utils::Result<SystemLoadAverage> getSystemLoadAverage() const;` but no `ProcessAnalyzer::getSystemLoadAverage` definition exists in src/; SystemLoadAverage (include/analyzer/system_model.h:24) has oneMin/fiveMin/fifteenMin.
      Surfaces: src/analyzer/system.cpp
      New Surfaces: tests/analyzer/system_load_average.cpp
      Verification: ctest --test-dir build -R "GetSystemLoadAverageTest" --output-on-failure

- [x] Implement getSystemActivityStats (declared but missing)
      Mode: develop
      Rationale: getSystemActivityStats is declared in the public API with no implementation; MockProc::createSystemStat already writes the ctxt and processes fields.
      Evidence: include/analyzer/core.h:61 declares the method; no definition in src/.
      Surfaces: src/analyzer/system.cpp
      New Surfaces: tests/analyzer/system_activity_stats.cpp
      Verification: ctest --test-dir build -R "GetSystemActivityStatsTest" --output-on-failure

- [x] Implement getSystemDiskIoStats (declared but missing)
      Mode: develop
      Rationale: getSystemDiskIoStats is declared in the public API with no implementation; DiskIoDeviceStats has all 11 I/O counter fields.
      Evidence: include/analyzer/core.h:59 declares the method; no definition in src/.
      Surfaces: src/analyzer/system.cpp
      New Surfaces: tests/analyzer/system_disk_io_stats.cpp
      Verification: ctest --test-dir build -R "GetSystemDiskIoStatsTest" --output-on-failure

- [x] Implement getProcessResourceLimits (declared but missing)
      Mode: develop
      Rationale: getProcessResourceLimits is declared in the public API with no implementation.
      Evidence: include/analyzer/core.h:47 declares the method; no definition in src/.
      Surfaces: src/analyzer/details.cpp
      New Surfaces: tests/analyzer/resource_limits.cpp
      Verification: ctest --test-dir build -R "GetProcessResourceLimitsTest" --output-on-failure

- [x] Implement getProcessCgroupInfo (declared but missing)
      Mode: develop
      Rationale: getProcessCgroupInfo is declared in the public API with no implementation.
      Evidence: include/analyzer/core.h:48 declares the method; no definition in src/.
      Surfaces: src/analyzer/details.cpp
      New Surfaces: tests/analyzer/cgroup_info.cpp
      Verification: ctest --test-dir build -R "GetProcessCgroupInfoTest" --output-on-failure

- [x] Implement getSystemDiskUsage (declared but missing)
      Mode: develop
      Rationale: getSystemDiskUsage is declared in the public API with no implementation.
      Evidence: include/analyzer/core.h:58 declares the method; no definition in src/.
      Surfaces: src/analyzer/system.cpp
      New Surfaces: tests/analyzer/system_disk_usage.cpp
      Verification: ctest --test-dir build -R "GetSystemDiskUsageTest" --output-on-failure

- [x] Implement getSystemInfo (declared but missing)
      Mode: develop
      Rationale: getSystemInfo is declared in the public API with no implementation.
      Evidence: include/analyzer/core.h:53 declares the method; no definition in src/.
      Surfaces: src/analyzer/system.cpp
      New Surfaces: tests/analyzer/system_info.cpp
      Verification: ctest --test-dir build -R "GetSystemInfoTest" --output-on-failure

- [x] Make streamQueryProcesses truly lazy for filter-only queries
      Mode: improve
      Rationale: streamQueryProcesses buffered all results before yielding; filter-only queries should stream lazily.
      Evidence: src/analyzer/base.cpp:207 called queryProcesses() inside the coroutine body.
      Surfaces: src/analyzer/base.cpp
      New Surfaces: tests/analyzer/stream_query.cpp
      Verification: ctest --test-dir build -R "StreamQueryProcessesTest" --output-on-failure

- [x] Implement getProcessMemoryMaps (declared but missing)
      Mode: develop
      Rationale: getProcessMemoryMaps is declared in the public API and the README advertises memory-map inspection, but no implementation exists, so the symbol is undefined and the feature is unusable; the MemoryMapInfo model and MockProc::createMaps already exist.
      Evidence: include/analyzer/core.h:46 declares `utils::Result<std::vector<MemoryMapInfo>> getProcessMemoryMaps(int pid) const;` with no definition in src/ (grep finds zero; nm shows it absent from libprocessAnalyzerLib.a); MemoryMapInfo (include/analyzer/process_model.h:51) has startAddress/endAddress/permissions/offset/device/inode/pathname, and MockProc::createMaps writes the matching /proc/<pid>/maps lines (tests/utils/testing_framework.cpp:293).
      Surfaces: src/analyzer/details.cpp
      New Surfaces: tests/analyzer/memory_maps.cpp
      Development: Implement ProcessAnalyzer::getProcessMemoryMaps in src/analyzer/details.cpp after getProcessEnvironment: checkPidPathExistsAndPermissions then read (procPath / std::to_string(pid) / "maps") with utils::readTextFile. For each line use std::istringstream to read addressRange, perms, offset, dev, inode with >>; split addressRange on '-' and convert both halves with utils::parseInteger<uint64_t>(.., 16); convert offset with base 16 and inode with base 10; then std::getline the remainder of the stream and utils::trim it into pathname (which may be empty or a pseudo-path like "[heap]"). Skip malformed lines. Create tests/analyzer/memory_maps.cpp: build a process, mockProc->createMaps(pid, {entry}) with a known ProcMapEntry plus one with an empty pathname, and assert the parsed MemoryMapInfo fields (startAddress, endAddress, permissions, offset, device, inode, pathname) and that the empty-pathname entry yields an empty pathname; assert an absent pid returns an error.
      Acceptance: getProcessMemoryMaps parses /proc/<pid>/maps into MemoryMapInfo entries; the new GetProcessMemoryMapsTest passes and all existing tests still pass.
      Verification: ctest --test-dir build -R "GetProcessMemoryMapsTest" --output-on-failure

- [x] Expand stringToProcessSortField with the 9 missing CLI sort-field mappings
      Mode: repair
      Rationale: stringToProcessSortField in args.cpp only mapped 10 of the 19 ProcessSortField enum values; the other 9 were supported by queryProcesses but unreachable via --sort-by.
      Evidence: src/cli/args.cpp:24-37 mapped only pid, ppid, uid, user, name, state, rss, vm, threads, start-time.
      Surfaces: src/cli/args.cpp, tests/cli/args.cpp
      Acceptance: All 19 sort fields accessible via --sort-by; 10 new ArgsTest cases pass (378 total).
      Verification: ctest --test-dir build -R "ArgsTest" --output-on-failure

- [x] Fix getSystemBootTimeUnix hardcoded /proc/stat path — make it use procPath
      Mode: repair
      Rationale: getSystemBootTimeUnix hardcoded "/proc/stat", making it the only ProcessAnalyzer accessor that couldn't be tested with MockProc.
      Evidence: src/analyzer/system.cpp:41 used a string literal instead of procPath/"stat".
      Surfaces: include/analyzer/core.h, src/analyzer/system.cpp
      New Surfaces: tests/analyzer/system_boot_time.cpp
      Acceptance: getSystemBootTimeUnix is now a const member reading procPath/"stat"; 3 new GetSystemBootTimeTest cases pass.
      Verification: ctest --test-dir build -R "GetSystemBootTimeTest" --output-on-failure

- [x] Fix getSystemInfo reading procPath/"etc/os-release" — use /etc/os-release first
      Mode: repair
      Rationale: getSystemInfo read procPath/"etc/os-release" (= /proc/etc/os-release, nonexistent), so osName always fell back to "Linux" on real systems.
      Evidence: src/analyzer/system.cpp:241 with comment acknowledging it was a testability workaround.
      Surfaces: src/analyzer/system.cpp, tests/analyzer/system_info.cpp
      Acceptance: Production reads real /etc/os-release; MockProc tests unchanged; 5 GetSystemInfoTest cases pass.
      Verification: ctest --test-dir build -R "GetSystemInfoTest" --output-on-failure

- [x] Remove the dead --config-file CLI option
      Mode: improve
      Rationale: --config-file was parsed and documented but main.cpp never read configFilePath — silently ignored.
      Evidence: src/cli/args.h:31 declared configFilePath; src/main.cpp had zero references.
      Surfaces: include/cli/args.h, src/cli/args.cpp, tests/cli/args.cpp
      Acceptance: --config-file is now rejected as an unknown option; ParseCommandLineRejectsConfigFile test passes.
      Verification: ctest --test-dir build -R "ArgsTest" --output-on-failure

- [x] Implement sendSignal in control.cpp
      Mode: develop
      Rationale: control.cpp was an empty stub; sendSignal delivers the Process Control feature with typed errors for ESRCH/EPERM/EINVAL.
      Evidence: src/analyzer/control.cpp contained only the copyright header.
      Surfaces: include/analyzer/core.h, src/analyzer/control.cpp
      New Surfaces: tests/analyzer/process_control.cpp
      Acceptance: sendSignal(getpid(), SIGCONT) succeeds; invalid-signal and absent-pid cases return correct typed errors; 3 SendSignalTest cases pass.
      Verification: ctest --test-dir build -R "SendSignalTest" --output-on-failure

- [x] Implement setProcessPriority in performance.cpp
      Mode: develop
      Rationale: performance.cpp was an empty stub; setProcessPriority delivers the Process Performance feature via setpriority(2) with typed errors.
      Evidence: src/analyzer/performance.cpp contained only the copyright header.
      Surfaces: include/analyzer/core.h, src/analyzer/performance.cpp
      New Surfaces: tests/analyzer/process_performance.cpp
      Acceptance: setProcessPriority(getpid(), 5) succeeds; absent-pid returns error; 2 SetProcessPriorityTest cases pass.
      Verification: ctest --test-dir build -R "SetProcessPriorityTest" --output-on-failure

- [x] Add a system subcommand to the CLI exposing system-wide metrics
      Mode: develop
      Rationale: Six system-wide library methods (getSystemInfo, getSystemLoadAverage, getSystemMemoryInfo, getSystemDiskUsage, etc.) were library-only with no CLI interface.
      Evidence: include/analyzer/core.h:53-62 declared the accessors; src/main.cpp handled only list/show/pid/name/user.
      Surfaces: src/cli/args.cpp, src/main.cpp
      Acceptance: processAnalyzer system outputs hostname, kernel, OS name, uptime, load averages, memory, and disk usage; all 378 tests pass.
      Verification: ctest --test-dir build --output-on-failure && ./build/processAnalyzer system

