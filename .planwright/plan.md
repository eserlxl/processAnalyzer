# planwright Plan — .
<!-- Session: 2026-06-04T20:00:00Z -->

- [x] Expand stringToProcessSortField with the 9 missing CLI sort-field mappings
      Mode: repair
      Rationale: stringToProcessSortField in args.cpp only maps 10 of the 19 ProcessSortField enum values; the other 9 (cmdline, executablePath, cwd, cpuTime, cpuUserTime, cpuKernelTime, ioReadBytes, ioWriteBytes, priority) are supported by queryProcesses but unreachable via --sort-by, so a user who passes --sort-by=priority gets "Error: Invalid sort field" even though the library handles it correctly.
      Evidence: src/cli/args.cpp:24-37 maps only pid, ppid, uid, user, name, state, rss, vm, threads, start-time; src/analyzer/query.cpp:123-143 has compareLess cases for all 19 ProcessSortField values.
      Surfaces: src/cli/args.cpp, tests/cli/args.cpp
      Verification: ctest --test-dir build -R "ArgsTest" --output-on-failure

- [x] Fix getSystemBootTimeUnix hardcoded /proc/stat path — make it use procPath
      Mode: repair
      Rationale: getSystemBootTimeUnix is a static method that hardcodes "/proc/stat"; every other system-data accessor in ProcessAnalyzer reads from procPath, so this method is the only one that cannot be exercised with a MockProc.
      Evidence: src/analyzer/system.cpp:41 calls utils::readTextFile("/proc/stat") — a string literal, not procPath / "stat".
      Surfaces: include/analyzer/core.h, src/analyzer/system.cpp
      New Surfaces: tests/analyzer/system_boot_time.cpp
      Verification: ctest --test-dir build -R "GetSystemBootTimeTest" --output-on-failure

- [x] Fix getSystemInfo reading procPath/"etc/os-release" — use /etc/os-release first
      Mode: repair
      Rationale: getSystemInfo constructs the os-release path as procPath/"etc/os-release"; on a real system this resolves to /proc/etc/os-release (nonexistent), so osName always falls back to "Linux" instead of the actual distro name.
      Evidence: src/analyzer/system.cpp:241 reads (procPath / "etc" / "os-release").
      Surfaces: src/analyzer/system.cpp, tests/analyzer/system_info.cpp
      Verification: ctest --test-dir build -R "GetSystemInfoTest" --output-on-failure

- [x] Remove the dead --config-file CLI option
      Mode: improve
      Rationale: The --config-file option is declared, parsed, and documented but main.cpp never reads args.configFilePath — it has no effect.
      Evidence: src/cli/args.h:31 declares configFilePath; src/main.cpp never references it.
      Surfaces: include/cli/args.h, src/cli/args.cpp, tests/cli/args.cpp
      Verification: ctest --test-dir build -R "ArgsTest" --output-on-failure

- [x] Implement sendSignal in control.cpp
      Mode: develop
      Rationale: control.cpp was an empty stub; sendSignal completes the Process Control feature with a typed-error wrapper around kill(2).
      Evidence: src/analyzer/control.cpp was header-only.
      Surfaces: include/analyzer/core.h, src/analyzer/control.cpp
      New Surfaces: tests/analyzer/process_control.cpp
      Verification: ctest --test-dir build -R "SendSignalTest" --output-on-failure

- [x] Implement setProcessPriority in performance.cpp
      Mode: develop
      Rationale: performance.cpp was an empty stub; setProcessPriority completes the Process Performance feature with a typed-error wrapper around setpriority(2).
      Evidence: src/analyzer/performance.cpp was header-only.
      Surfaces: include/analyzer/core.h, src/analyzer/performance.cpp
      New Surfaces: tests/analyzer/process_performance.cpp
      Verification: ctest --test-dir build -R "SetProcessPriorityTest" --output-on-failure

- [x] Add a system subcommand to the CLI exposing system-wide metrics
      Mode: develop
      Rationale: The ProcessAnalyzer library has a rich system-wide API (getSystemInfo, getSystemLoadAverage, getSystemMemoryInfo, getSystemDiskUsage) but the CLI had no way to access it.
      Evidence: include/analyzer/core.h declares six system-wide accessors; src/main.cpp handled only list/show/pid/name/user commands.
      Surfaces: src/cli/args.cpp, src/main.cpp
      Verification: ctest --test-dir build --output-on-failure && ./build/processAnalyzer system

- [x] Remove dead `default:` case in `compareLess` — restore compile-time safety for new sort fields
      Mode: repair
      Rationale: The `compareLess` lambda in query.cpp:143 has `default: return a.pid < b.pid` even though all 19 ProcessSortField enum values are already handled by named cases. With `-Wswitch` in effect, removing the default makes the compiler emit a warning (escalated to error by -Werror) if any future enum value is added without a matching case, preventing silent fallback to pid ordering.
      Evidence: src/analyzer/query.cpp:143 `default: return a.pid < b.pid`; lines 124-142 handle all 19 ProcessSortField values explicitly.
      Surfaces: src/analyzer/query.cpp
      Verification: cmake --build build && ctest --test-dir build -R "QueryProcesses" --output-on-failure

- [x] Remove dead `testConfigFileCustom` constant from ArgsTest fixture
      Mode: improve
      Rationale: tests/cli/args.cpp:42 defines `testConfigFileCustom = "my_config.ini"` but no test references it after the --config-file option was removed in the previous cycle; the constant is dead and misleads readers into thinking a config-file code path still exists.
      Evidence: grep -n "testConfigFileCustom" tests/cli/args.cpp → only line 42 (definition, never used).
      Surfaces: tests/cli/args.cpp
      Verification: ctest --test-dir build -R "ArgsTest" --output-on-failure

- [x] Expand `pseudoFsTypes` in getSystemDiskUsage to filter fuse and binfmt_misc entries
      Mode: repair
      Rationale: getSystemDiskUsage at system.cpp:176-180 already filters common pseudo-filesystems but misses `binfmt_misc` (whose mount point `/proc/sys/fs/binfmt_misc` statvfs can successfully query) and all `fuse.*` types (`fuse.portal`, `fuse.gvfsd-fuse`, etc.) whose statvfs returns the backing directory's capacity, causing confusing duplicate disk-space entries in the output.
      Evidence: /proc/mounts on this system shows `binfmt_misc /proc/sys/fs/binfmt_misc binfmt_misc` and `fuse.portal`/`fuse.gvfsd-fuse` entries; none are in system.cpp:176-180 pseudoFsTypes; `overlay` (Docker/podman) and `squashfs` (snap) also absent.
      Surfaces: src/analyzer/system.cpp, tests/analyzer/system_disk_usage.cpp
      Verification: ctest --test-dir build -R "GetSystemDiskUsageTest" --output-on-failure

- [x] Filter zero-totalSpace entries from getSystemDiskUsage result
      Mode: repair
      Rationale: Even after filtering by filesystem type, some virtual mounts (e.g. tmpfs-backed credentials dirs, loop mounts not yet populated) have `f_blocks == 0` from statvfs, producing useless "0.0 GiB" rows in the system disk-usage output. Skipping entries where totalSpaceBytes == 0 keeps the result actionable.
      Evidence: src/analyzer/system.cpp:197-200 sets totalSpaceBytes = sv.f_blocks * sv.f_frsize but never guards against sv.f_blocks == 0; real /proc/mounts contains tmpfs entries with tiny or zero block counts.
      Surfaces: src/analyzer/system.cpp, tests/analyzer/system_disk_usage.cpp
      Verification: ctest --test-dir build -R "GetSystemDiskUsageTest" --output-on-failure

- [x] Add "cwd" as a selectable table column
      Mode: develop
      Rationale: ProcessInfo::currentWorkingDirectory is populated for every process (from /proc/[pid]/cwd) but cannot be requested via --columns; `cwd` is absent from validColumns in args.cpp, getProcessInfoValue in output.cpp, the defaultColumnWidths map, and printVerticalProcessDetails.
      Evidence: src/cli/args.cpp:13-17 validColumns array has 14 entries; "cwd" not among them. src/cli/output.cpp:18-36 getProcessInfoValue has no "cwd" branch. src/cli/output.cpp:72-75 printVerticalProcessDetails does not print currentWorkingDirectory.
      Surfaces: src/cli/args.cpp, src/cli/output.cpp, tests/cli/args.cpp, tests/cli/output.cpp
      Verification: ctest --test-dir build -R "ArgsTest|OutputTest" --output-on-failure

- [x] Show IO read/write stats in printVerticalProcessDetails
      Mode: develop
      Rationale: ProcessInfo::ioReadBytes and ioWriteBytes are populated from /proc/[pid]/io for every process where the kernel permits it, but printVerticalProcessDetails in output.cpp never renders them, so `show`/`pid` command output is missing the I/O section entirely.
      Evidence: src/cli/output.cpp:72-83 printVerticalProcessDetails prints PID through Command but has no IO Read/Write lines; grep for ioReadBytes in output.cpp returns nothing.
      Surfaces: src/cli/output.cpp, tests/cli/output.cpp
      Verification: ctest --test-dir build -R "OutputTest" --output-on-failure

- [x] Add --min-rss / --max-rss CLI filter options
      Mode: develop
      Rationale: ProcessFilter supports minResidentMemoryKB / maxResidentMemoryKB (process_model.h) and queryProcesses applies them (query.cpp:75), but parseCommandLine has no --min-rss or --max-rss flags, so users cannot filter processes by memory usage from the CLI even though the library fully supports it.
      Evidence: include/analyzer/process_model.h declares minResidentMemoryKB/maxResidentMemoryKB; grep for "min-rss\|max-rss" in src/cli/args.cpp returns nothing.
      Surfaces: include/cli/args.h, src/cli/args.cpp, src/main.cpp, tests/cli/args.cpp
      Verification: ctest --test-dir build -R "ArgsTest" --output-on-failure

- [x] Extract shared filter predicate to eliminate duplicate filter logic in query.cpp and base.cpp
      Mode: improve
      Rationale: query.cpp:66-84 and base.cpp:215-253 apply 12 identical ProcessFilter conditions independently; when a new filter field is added it must be updated in both places — the recent --min-rss addition required edits in both files. Extracting the shared logic into a helper reduces this to a single edit site.
      Evidence: src/analyzer/query.cpp:66-84 applies filters via applyStringFilter/applyRangeFilter; src/analyzer/base.cpp:215-253 duplicates the same 12 conditions inline.
      Surfaces: src/analyzer/query.cpp, src/analyzer/base.cpp
      New Surfaces: src/analyzer/internal/filter_helpers.h
      Verification: cmake --build build && ctest --test-dir build --output-on-failure

- [x] Add missing-value error tests for range filter CLI options
      Mode: improve
      Rationale: --min-rss, --max-rss, --min-threads, --max-threads each guard against missing values with an error message, but this path is not exercised by any test — only the invalid-type path for --min-rss is covered.
      Evidence: tests/cli/args.cpp has MinRssInvalidValueReturnsNullopt but no tests for missing-value paths for any of the four new options; args.cpp guards each at i+1>=cliArgs.size().
      Surfaces: tests/cli/args.cpp
      Verification: ctest --test-dir build -R "ArgsTest" --output-on-failure

- [x] Add test coverage for networkConnectionFilter in queryProcesses
      Mode: improve
      Rationale: ProcessFilter.networkConnectionFilter is implemented in query.cpp:87-112 and base.cpp:235-251 but no test constructs a filter with this field set; the multi-criterion matching logic (localPort, remotePort, protocol, state, remoteAddressContains) is completely untested.
      Evidence: grep networkConnectionFilter tests/ returns only args-parsing references, never a filter construction; tests/analyzer/query.cpp has no network filter test.
      Surfaces: tests/analyzer/query.cpp
      Verification: ctest --test-dir build -R "QueryProcesses" --output-on-failure

- [x] Add --uid <N> CLI filter flag
      Mode: develop
      Rationale: ProcessFilter.uidFilter is defined and applied in queryProcesses but the CLI exposes only --user (username) with no way to filter by numeric UID; numeric UID filtering is natural in scripts and avoids a name-lookup round-trip.
      Evidence: include/analyzer/process_model.h:uidFilter; grep "--uid\|uidFilter" src/cli/args.cpp returns nothing.
      Surfaces: include/cli/args.h, src/cli/args.cpp, src/main.cpp, tests/cli/args.cpp
      Verification: ctest --test-dir build -R "ArgsTest" --output-on-failure

- [x] Show CPU user/kernel time in printVerticalProcessDetails
      Mode: develop
      Rationale: ProcessInfo::cpuUserTimeTicks and cpuKernelTimeTicks are populated from /proc/[pid]/stat for every process but printVerticalProcessDetails never renders them — CPU time is a key diagnostic field absent from the single-process view.
      Evidence: src/cli/output.cpp:83-99 printVerticalProcessDetails has no CPU time lines; grep cpuUserTimeTicks in output.cpp returns nothing.
      Surfaces: src/cli/output.cpp, tests/cli/output.cpp
      Verification: ctest --test-dir build -R "OutputTest" --output-on-failure

- [x] Add network interface stats section to `system` command output
      Mode: develop
      Rationale: getNetworkInterfaceStats is implemented and tested but the `system` command does not call it; users querying system-wide information have no way to see interface RX/TX stats from the CLI.
      Evidence: include/analyzer/core.h declares getNetworkInterfaceStats; src/main.cpp system branch does not call it.
      Surfaces: src/main.cpp
      Verification: cmake --build build && ./build/processAnalyzer system

- [x] Add disk I/O stats and system activity section to `system` command output
      Mode: develop
      Rationale: getSystemDiskIoStats and getSystemActivityStats are both implemented and tested but absent from the `system` command; disk I/O counters and context-switch/interrupt counts complete the system-monitoring picture.
      Evidence: tests/analyzer/system_disk_io_stats.cpp and system_activity_stats.cpp exist; grep getSystemDiskIoStats in main.cpp returns nothing.
      Surfaces: src/main.cpp
      Verification: cmake --build build && ./build/processAnalyzer system
