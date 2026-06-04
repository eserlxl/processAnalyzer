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
