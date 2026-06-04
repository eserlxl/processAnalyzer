# planwright Plan — .
<!-- Session: 2026-06-05T03:00:00Z -->

## Cycle 11 — Coverage (depth 10)

Audit found: `tests/utils/system.cpp` is empty (0 tests) despite `src/utils/system.cpp`
having 5 public functions (182 lines): `getEnv`, `setEnv`, `unsetEnv`,
`getCurrentWorkingDirectory`, `executeCommand`. Also `getSystemMemoryInfo` has no
partial-fields (swap-absent) test.

---

### Item 1 — `utils::getEnv` test coverage
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/utils/system.cpp`  
**Surfaces:** `src/utils/system.cpp:27-41` (`getEnv` implementation)  
**Description:** `utils::getEnv` has four code paths: (a) empty name → `invalidArgument`;
(b) name contains `'='` → `invalidArgument`; (c) name contains `'\0'` → `invalidArgument`;
(d) nonexistent var → `envVarNotFound`; (e) existing var → returns its value. None are
tested. Add `TEST(SystemUtilsTest, GetEnvRejectsEmptyName)`,
`TEST(SystemUtilsTest, GetEnvRejectsEqualsInName)`,
`TEST(SystemUtilsTest, GetEnvRejectsNullByteInName)`,
`TEST(SystemUtilsTest, GetEnvReturnsErrorForMissingVar)`,
`TEST(SystemUtilsTest, GetEnvReturnsValueForExistingVar)`.  
The test for (e) should use `setenv("PROC_ANALYZER_TEST_VAR", "hello", 1)` before the
call and `unsetenv` after (in fixture `SetUp`/`TearDown`) to avoid side effects.  
**Verification:** Build + ctest passes. 5 new tests all pass.  
**Status:** [x] done

---

### Item 2 — `utils::setEnv` / `utils::unsetEnv` test coverage
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/utils/system.cpp`  
**Surfaces:** `src/utils/system.cpp:128-157` (`setEnv`, `unsetEnv` implementations)  
**Description:** `setEnv` validates name (empty, '=', '\0'), sets the variable, and
`unsetEnv` clears it. Neither has any test. Add:
`TEST(SystemUtilsTest, SetEnvAndGetEnvRoundTrip)` — set `PROC_ANALYZER_ROUND_TRIP=value`,
then `getEnv` returns `"value"`;
`TEST(SystemUtilsTest, UnsetEnvRemovesVar)` — set, then unset, then `getEnv` returns error;
`TEST(SystemUtilsTest, SetEnvRejectsEmptyName)` — empty name → `invalidArgument`;
`TEST(SystemUtilsTest, SetEnvRejectsEqualsInName)` — `"A=B"` as name → `invalidArgument`.  
**Verification:** Build + ctest passes. 4 new tests all pass.  
**Status:** [x] done

---

### Item 3 — `utils::getCurrentWorkingDirectory` test coverage
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/utils/system.cpp`  
**Surfaces:** `src/utils/system.cpp:160-169` (`getCurrentWorkingDirectory` implementation)  
**Description:** `getCurrentWorkingDirectory` wraps `getcwd()` and has no test. Add
`TEST(SystemUtilsTest, GetCurrentWorkingDirectoryReturnsValidPath)` asserting: result
has value, the path is absolute, and `std::filesystem::exists(result.value())` is true.  
**Verification:** Build + ctest passes. New test passes.  
**Status:** [x] done

---

### Item 4 — `utils::executeCommand` test coverage
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/utils/system.cpp`  
**Surfaces:** `src/utils/system.cpp:43-129` (`executeCommand` implementation)  
**Description:** `executeCommand` has 0 tests. The empty-command path (returns
`invalidArgument`) and the successful execution path (exit code 0, captured stdout) are
the two key branches. Add:
`TEST(SystemUtilsTest, ExecuteCommandRejectsEmptyCommand)` — empty string → `invalidArgument`;
`TEST(SystemUtilsTest, ExecuteCommandCapturesStdout)` — `echo hello` → exit code 0, stdout
contains `"hello"`.  
**Verification:** Build + ctest passes. Both new tests pass.  
**Status:** [x] done

---

### Item 5 — `getSystemMemoryInfo` with absent swap fields
**Mode:** improve  
**Rung:** 2 (coverage)  
**File:** `tests/analyzer/system_memory.cpp`  
**Surfaces:** `src/analyzer/system.cpp:58-96` (`getSystemMemoryInfo` — swap fields are
optional and default to 0 if absent from the file)  
**Description:** The existing `ParsesMeminfoFields` test covers a complete meminfo with
swap. When `SwapTotal:` and `SwapFree:` are absent from the file (e.g., a system with
no swap), the code leaves `swapTotal` and `swapFree` as 0 — the default `unsigned long`
value. This branch is never tested. Add
`GetSystemMemoryInfoTest.ParsesMeminfoWithoutSwapFields`: provide a meminfo with
`MemTotal`/`MemFree`/`MemAvailable`/`Buffers`/`Cached` but no swap lines, then assert
result succeeds and `swapTotal == 0` and `swapFree == 0`.  
**Verification:** Build + ctest passes. New test passes and pins the swap-absent behaviour.  
**Status:** [x] done

