// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "cli/args.h" // Includes analyzer/process_model.h indirectly

#include <vector>
#include <string>
#include <optional>
#include <span>

// Define a test fixture to manage argument buffers and ensure thread-safety.
class ArgsTestFixture : public ::testing::Test {
protected:
    // PID values
    constexpr static int testPid = 1234;
    constexpr static int testPpid = 5678;
    constexpr static int testPidReject = 42;
    constexpr static int testPpidReject = 1;
    constexpr static const char* maxLlStr = "9223372036854775807";
    constexpr static const char* minLlStr = "-9223372036854775807";

    // Name values
    constexpr static const char* testNameFirefox = "firefox";
    constexpr static const char* testNameChrome = "chrome";

    // User values
    constexpr static const char* testUserRoot = "root";
    constexpr static const char* testUserUser1 = "user1";

    // Output formats
    constexpr static const char* testOutputJson = "json";
    constexpr static const char* testOutputVertical = "vertical";
    constexpr static const char* testOutputCsv = "csv";

    // State filter
    constexpr static char testStateR = 'R';
    constexpr static char testStateS = 'S';

    // Dummy path used to test that --config-file is rejected as an unknown option.
    constexpr static const char* testConfigFileDefault = "/etc/processAnalyzer.conf";

    // Other filter values
    constexpr static int testPpidMultipleOptions = 1000;
    constexpr static int testPidGeneric = 123;


    // Helper to convert std::vector<std::string> to std::vector<char*> for argv.
    // Creates mutable copies of the strings to avoid potential undefined behavior
    // if parseCommandLine were to modify the contents of argv.
    // This method uses the fixture's member 'argBuffers_' for thread-safe storage.
    std::vector<char*> makeArgv(const std::vector<std::string>& args) {
        argBuffers.clear(); // Clear previous arguments to reuse the buffer
        argBuffers.reserve(args.size());

        for (const std::string& arg : args) {
            argBuffers.emplace_back(arg.begin(), arg.end());
            argBuffers.back().push_back('\0'); // Null-terminate the string
        }

        std::vector<char*> argv;
        argv.reserve(argBuffers.size());
        for (auto& buffer : argBuffers) {
            argv.push_back(buffer.data()); // Get pointer to the managed buffer
        }
        return argv; // Return pointers to the managed data
    }

    // Use TEST_F macro in tests below, which will automatically call SetUp()
    // and TearDown() for each test.
    void SetUp() override {
        // Initialization for each test if needed.
        // For now, makeArgv clears argBuffers_ at the start of each call,
        // so explicit clearing here is not strictly necessary but good practice.
        argBuffers.clear();
    }

    void TearDown() override {
        // Cleanup for each test if needed.
        argBuffers.clear(); // Ensure buffer is clean after test
    }

private:
    // Buffer storage for argument strings. This is a member of the fixture,
    // making it thread-safe when tests are run in parallel, as each test
    // instance will have its own 'argBuffers_'.
    std::vector<std::vector<char>> argBuffers;
};

// Use TEST_F macro for all tests to associate them with the fixture.
TEST_F(ArgsTestFixture, ParseCommandLineBasic) {
    std::vector<std::string> args = {"processAnalyzer", "list"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        EXPECT_FALSE(parsedArgs.value().pid.has_value());
        EXPECT_FALSE(parsedArgs.value().name.has_value());
        EXPECT_FALSE(parsedArgs.value().user.has_value());
        EXPECT_FALSE(parsedArgs.value().briefMode);
        EXPECT_TRUE(parsedArgs.value().selectedColumns.empty());
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithPid) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", std::to_string(testPid)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "show");
        ASSERT_TRUE(parsedArgs.value().pid.has_value());
        EXPECT_EQ(parsedArgs.value().pid.value(), testPid);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithPidShort) {
    std::vector<std::string> args = {"processAnalyzer", "show", "-p", std::to_string(testPid)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "show");
        ASSERT_TRUE(parsedArgs.value().pid.has_value());
        EXPECT_EQ(parsedArgs.value().pid.value(), testPid);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithName) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--name", testNameFirefox};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        ASSERT_TRUE(parsedArgs.value().name.has_value());
        EXPECT_EQ(parsedArgs.value().name.value(), testNameFirefox);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithUser) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--user", testUserRoot};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        ASSERT_TRUE(parsedArgs.value().user.has_value());
        EXPECT_EQ(parsedArgs.value().user.value(), testUserRoot);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithBrief) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--brief"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        EXPECT_TRUE(parsedArgs.value().briefMode);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithColumns) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--columns", "pid,name"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        ASSERT_EQ(parsedArgs.value().selectedColumns.size(), 2);
        EXPECT_EQ(parsedArgs.value().selectedColumns[0], "pid");
        EXPECT_EQ(parsedArgs.value().selectedColumns[1], "name");
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithNoTruncateCmdline) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--no-truncate-cmdline"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        EXPECT_TRUE(parsedArgs.value().noTruncateCmdline);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithOutput) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--output", testOutputJson};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        ASSERT_TRUE(parsedArgs.value().outputFormat.has_value());
        EXPECT_EQ(parsedArgs.value().outputFormat.value(), testOutputJson);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithVerticalOutput) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", std::to_string(testPid), "--output", testOutputVertical};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "show");
        ASSERT_TRUE(parsedArgs.value().outputFormat.has_value());
        EXPECT_EQ(parsedArgs.value().outputFormat.value(), testOutputVertical);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithStateFilter) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--state", std::string(1, testStateR)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        ASSERT_TRUE(parsedArgs.value().stateFilter.has_value());
        EXPECT_EQ(parsedArgs.value().stateFilter.value(), testStateR);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithSortByAscending) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--sort-by", "pid"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        ASSERT_TRUE(parsedArgs.value().sortBy.has_value());
        EXPECT_EQ(parsedArgs.value().sortBy.value(), ProcessSortField::pid);
        EXPECT_EQ(parsedArgs.value().sortOrder, SortOrder::asc); // Default
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithSortByDescending) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--sort-by", "name", "--sort-order", "desc"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        ASSERT_TRUE(parsedArgs.value().sortBy.has_value());
        EXPECT_EQ(parsedArgs.value().sortBy.value(), ProcessSortField::name);
        EXPECT_EQ(parsedArgs.value().sortOrder, SortOrder::desc);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithHelp) {
    std::vector<std::string> args = {"processAnalyzer", "--help"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_TRUE(parsedArgs.value().showHelp);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithChildren) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", std::to_string(testPidGeneric), "--children"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "show");
        EXPECT_TRUE(parsedArgs.value().showChildren);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithOpenFiles) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", std::to_string(testPidGeneric), "--open-files"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "show");
        EXPECT_TRUE(parsedArgs.value().showOpenFiles);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithThreads) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", std::to_string(testPidGeneric), "--threads"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "show");
        EXPECT_TRUE(parsedArgs.value().showThreads);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithNetwork) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", std::to_string(testPidGeneric), "--network"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "show");
        EXPECT_TRUE(parsedArgs.value().showNetworkConnections);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithPpidFilter) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--ppid", std::to_string(testPpid)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        ASSERT_TRUE(parsedArgs.value().ppidFilter.has_value());
        EXPECT_EQ(parsedArgs.value().ppidFilter.value(), testPpid);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineRejectsConfigFile) {
    // --config-file was removed (it was parsed but silently ignored); now an unknown option.
    std::vector<std::string> args = {"processAnalyzer", "list", "--config-file", testConfigFileDefault};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineInvalidPid) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "abc"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineMissingPidValue) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineInvalidSortBy) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--sort-by", "invalid"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineInvalidSortOrder) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--sort-by", "pid", "--sort-order", "invalid"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineInvalidState) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--state", "RUNNING"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineUnknownCommand) {
    std::vector<std::string> args = {"processAnalyzer", "unknown_command"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineNoCommand) {
    std::vector<std::string> args = {"processAnalyzer"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineMultipleOptions) {
    std::vector<std::string> args = {
        "processAnalyzer", "list",
        "--name", testNameChrome,
        "-u", testUserUser1,
        "-s", std::string(1, testStateS),
        "--columns", "pid,name,cmdline",
        "--brief",
        "--no-truncate-cmdline",
        "--output", testOutputCsv,
        "--ppid", std::to_string(testPpidMultipleOptions),
    };
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        ASSERT_TRUE(parsedArgs.value().name.has_value());
        EXPECT_EQ(parsedArgs.value().name.value(), testNameChrome);
        ASSERT_TRUE(parsedArgs.value().user.has_value());
        EXPECT_EQ(parsedArgs.value().user.value(), testUserUser1);
        ASSERT_TRUE(parsedArgs.value().stateFilter.has_value());
        EXPECT_EQ(parsedArgs.value().stateFilter.value(), testStateS);
        ASSERT_EQ(parsedArgs.value().selectedColumns.size(), 3);
        EXPECT_EQ(parsedArgs.value().selectedColumns[0], "pid");
        EXPECT_EQ(parsedArgs.value().selectedColumns[1], "name");
        EXPECT_EQ(parsedArgs.value().selectedColumns[2], "cmdline");
        EXPECT_TRUE(parsedArgs.value().briefMode);
        EXPECT_TRUE(parsedArgs.value().noTruncateCmdline);
        ASSERT_TRUE(parsedArgs.value().outputFormat.has_value());
        EXPECT_EQ(parsedArgs.value().outputFormat, testOutputCsv);
        ASSERT_TRUE(parsedArgs.value().ppidFilter.has_value());
        EXPECT_EQ(parsedArgs.value().ppidFilter, testPpidMultipleOptions);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineRejectsInvalidColumns) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--columns", "pid,name,not-a-column"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, CwdIsValidColumn) {
    auto argv = makeArgv({"processAnalyzer", "list", "--columns", "pid,name,cwd"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_EQ(parsed->selectedColumns.size(), 3U);
    EXPECT_EQ(parsed->selectedColumns[2], "cwd");
}

TEST_F(ArgsTestFixture, ParseCommandLinePositionalPidCommand) {
    std::vector<std::string> args = {"processAnalyzer", "pid", std::to_string(testPid), "--threads"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "pid");
        ASSERT_TRUE(parsedArgs.value().pid.has_value());
        EXPECT_EQ(parsedArgs.value().pid.value(), testPid);
        EXPECT_TRUE(parsedArgs.value().showThreads);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLinePositionalPidCommandMissingValue) {
    std::vector<std::string> args = {"processAnalyzer", "pid"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLinePositionalPidCommandInvalidValue) {
    std::vector<std::string> args = {"processAnalyzer", "pid", "not-a-number"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLinePositionalNameCommand) {
    std::vector<std::string> args = {"processAnalyzer", "name", testNameFirefox};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs.value().command, "name");
    ASSERT_TRUE(parsedArgs.value().name.has_value());
    EXPECT_EQ(parsedArgs.value().name.value(), testNameFirefox);
}

TEST_F(ArgsTestFixture, ParseCommandLinePositionalUserCommand) {
    std::vector<std::string> args = {"processAnalyzer", "user", testUserRoot};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs.has_value()) {
        EXPECT_EQ(parsedArgs.value().command, "user");
        ASSERT_TRUE(parsedArgs.value().user.has_value());
        EXPECT_EQ(parsedArgs.value().user.value(), testUserRoot);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLinePidCommandRejectsPpidFilter) {
    std::vector<std::string> args = {"processAnalyzer", "pid", std::to_string(testPidReject), "--ppid", std::to_string(testPpidReject)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineRejectsOutOfRangeLongPid) {
    // Using a value that exceeds typical int limits for PID.
    // The exact limit depends on the system, but this should be large enough
    // to test potential overflow or validation issues.
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", maxLlStr}; // Max long long
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineRejectsOutOfRangePositionalPid) {
    // Using a value that exceeds typical int limits for PID.
    std::vector<std::string> args = {"processAnalyzer", "pid", maxLlStr}; // Max long long
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineRejectsOutOfRangePpid) {
    // Using a value that exceeds typical int limits for PPID.
    std::vector<std::string> args = {"processAnalyzer", "list", "--ppid", minLlStr}; // Min long long
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

// Tests for the expanded sort-field mappings added in Cycle 2
TEST_F(ArgsTestFixture, SortByCmdline) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "cmdline"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::cmdline);
}

TEST_F(ArgsTestFixture, SortByExecPath) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "exec-path"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::executablePath);
}

TEST_F(ArgsTestFixture, SortByCwd) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "cwd"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::cwd);
}

TEST_F(ArgsTestFixture, SortByCpuTime) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "cpu-time"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::cpuTime);
}

TEST_F(ArgsTestFixture, SortByCpuUserTime) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "cpu-user-time"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::cpuUserTime);
}

TEST_F(ArgsTestFixture, SortByCpuKernelTime) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "cpu-kernel-time"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::cpuKernelTime);
}

TEST_F(ArgsTestFixture, SortByIoRead) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "io-read"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::ioReadBytes);
}

TEST_F(ArgsTestFixture, SortByIoWrite) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "io-write"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::ioWriteBytes);
}

TEST_F(ArgsTestFixture, SortByPriority) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "priority"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::priority);
}

TEST_F(ArgsTestFixture, SortByUnknownFieldReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "not-a-field"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_FALSE(parsed.has_value());
}

TEST_F(ArgsTestFixture, MinRssIsSet) {
    auto argv = makeArgv({"processAnalyzer", "list", "--min-rss", "1024"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->minRssKb.has_value());
    EXPECT_EQ(parsed->minRssKb.value(), 1024LL);
}

TEST_F(ArgsTestFixture, MaxRssIsSet) {
    auto argv = makeArgv({"processAnalyzer", "list", "--max-rss", "8192"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->maxRssKb.has_value());
    EXPECT_EQ(parsed->maxRssKb.value(), 8192LL);
}

TEST_F(ArgsTestFixture, MinRssInvalidValueReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--min-rss", "notanumber"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_FALSE(parsed.has_value());
}

TEST_F(ArgsTestFixture, MinThreadsIsSet) {
    auto argv = makeArgv({"processAnalyzer", "list", "--min-threads", "4"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->minThreads.has_value());
    EXPECT_EQ(parsed->minThreads.value(), 4L);
}

TEST_F(ArgsTestFixture, MaxThreadsIsSet) {
    auto argv = makeArgv({"processAnalyzer", "list", "--max-threads", "16"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->maxThreads.has_value());
    EXPECT_EQ(parsed->maxThreads.value(), 16L);
}

TEST_F(ArgsTestFixture, MinRssMissingValueReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--min-rss"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, MaxRssMissingValueReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--max-rss"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, MinThreadsMissingValueReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--min-threads"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, MaxThreadsMissingValueReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--max-threads"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, MaxRssInvalidValueReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--max-rss", "notanumber"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, MinThreadsInvalidValueReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--min-threads", "notanumber"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, UidFilterIsSet) {
    constexpr int kTestUid = 1000;
    auto argv = makeArgv({"processAnalyzer", "list", "--uid", "1000"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->uidFilter.has_value());
    EXPECT_EQ(parsed->uidFilter.value(), kTestUid);
}

TEST_F(ArgsTestFixture, UidFilterMissingValueReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--uid"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, UidFilterInvalidValueReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--uid", "notanumber"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}
