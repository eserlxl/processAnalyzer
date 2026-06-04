# planwright digest — routing only (UNVERIFIED)

UNVERIFIED — routing only
cluster 0 [tests] — 73 members — Core project: headers (core.h, process_model.h, system_model.h, network_model.h, utils/*), src/analyzer/* implementations, src/cli/*, tests/analyzer/*, tests/cli/*, docs/*. Highest-priority routing target. 7 public API methods declared in core.h but unimplemented: getSystemLoadAverage, getSystemActivityStats, getSystemDiskIoStats, getSystemDiskUsage, getSystemInfo, getProcessResourceLimits, getProcessCgroupInfo. streamQueryProcesses at base.cpp:207 not truly lazy.

UNVERIFIED — routing only
cluster 1 [(root)] — 1 members — .clang-format: code style config, singleton.

UNVERIFIED — routing only
cluster 2 [(root)] — 1 members — .clang-tidy: static analysis config, singleton.

UNVERIFIED — routing only
cluster 3 [(root)] — 1 members — .geminiignore: ignore rules, singleton.

UNVERIFIED — routing only
cluster 4 [(root)] — 1 members — .gitignore: VCS ignore rules, singleton.

UNVERIFIED — routing only
cluster 5 [(root)] — 1 members — CMakeLists.txt: root build config; links processAnalyzerLib + processAnalyzerCliLib; GLOB_RECURSE for test sources. In dirty set (change-coupled).

UNVERIFIED — routing only
cluster 6 [(root)] — 1 members — CMakePresets.json: CMake presets, singleton.

UNVERIFIED — routing only
cluster 7 [(root)] — 1 members — MISSION.yaml: project mission statement — "high-performance C++ CLI for real-time process inspection and monitoring, detailed resource usage metrics and execution statistics."

UNVERIFIED — routing only
cluster 8 [(root)] — 1 members — audit.md: prior audit notes, singleton.

UNVERIFIED — routing only
cluster 9 [(root)] — 1 members — audit2.md: prior audit notes, singleton.

UNVERIFIED — routing only
cluster 10 [(root)] — 1 members — gemini-cli-health.db: tool artifact, singleton.

UNVERIFIED — routing only
cluster 11 [src/analyzer] — 1 members — src/analyzer/control.cpp: empty stub (copyright header only); no corresponding core.h declarations exist post-redesign.

UNVERIFIED — routing only
cluster 12 [src/analyzer] — 1 members — src/analyzer/performance.cpp: empty stub (copyright header only); no corresponding core.h declarations exist.

UNVERIFIED — routing only
cluster 13 [tests] — 1 members — tests/CMakeLists.txt: test build config, GLOB_RECURSE all test .cpp files into single executable.

UNVERIFIED — routing only
cluster 14 [tests/analyzer] — 1 members — tests/utils/testing_framework.cpp: MockProc implementation; provides createStat, createMaps, createMeminfo, createUptime, createVersion, createNetDev, createEnviron, and ProcessBuilder fluent API.

UNVERIFIED — routing only
cluster 15 [tests/cli] — 1 members — tests/utils/testing_framework.h: MockProc header; exposes structs ProcStatData, ProcMapEntry, MeminfoData, SystemStatData, NetDevStats, AddThreadOptions.

UNVERIFIED — routing only
cluster 16 [tests/utils] — 1 members — tests/utils/file.cpp: util tests for file reading, singleton.

UNVERIFIED — routing only
cluster 17 [tests/utils] — 1 members — tests/utils/system.cpp: util tests for system helpers, singleton.
