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
    constexpr static int TEST_PID = 1234;
    constexpr static int TEST_PPID = 5678;
    constexpr static int TEST_PID_REJECT = 42;
    constexpr static int TEST_PPID_REJECT = 1;
    constexpr static const char* MAX_LL_STR = "9223372036854775807";
    constexpr static const char* MIN_LL_STR = "-9223372036854775807";

    // Name values
    constexpr static const char* TEST_NAME_FIREFOX = "firefox";
    constexpr static const char* TEST_NAME_CHROME = "chrome";

    // User values
    constexpr static const char* TEST_USER_ROOT = "root";
    constexpr static const char* TEST_USER_USER1 = "user1";

    // Output formats
    constexpr static const char* TEST_OUTPUT_JSON = "json";
    constexpr static const char* TEST_OUTPUT_VERTICAL = "vertical";
    constexpr static const char* TEST_OUTPUT_CSV = "csv";

    // State filter
    constexpr static char TEST_STATE_R = 'R';
    constexpr static char TEST_STATE_S = 'S';

    // Config file path
    constexpr static const char* TEST_CONFIG_FILE_DEFAULT = "/etc/processAnalyzer.conf";
    constexpr static const char* TEST_CONFIG_FILE_CUSTOM = "my_config.ini";

    // Other filter values
    constexpr static int TEST_PPID_MULTIPLE_OPTIONS = 1000;
    constexpr static int TEST_PID_GENERIC = 123;


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

    if (parsedArgs) {
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
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", std::to_string(TEST_PID)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "show");
        ASSERT_TRUE(parsedArgs->pid.has_value());
        EXPECT_EQ(*parsedArgs->pid, TEST_PID);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithPidShort) {
    std::vector<std::string> args = {"processAnalyzer", "show", "-p", std::to_string(TEST_PID)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "show");
        ASSERT_TRUE(parsedArgs->pid.has_value());
        EXPECT_EQ(*parsedArgs->pid, TEST_PID);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithName) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--name", TEST_NAME_FIREFOX};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->name.has_value());
        EXPECT_EQ(*parsedArgs->name, TEST_NAME_FIREFOX);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithUser) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--user", TEST_USER_ROOT};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->user.has_value());
        EXPECT_EQ(*parsedArgs->user, TEST_USER_ROOT);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithBrief) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--brief"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        EXPECT_TRUE(parsedArgs.value().briefMode);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithColumns) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--columns", "pid,name,cpu"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        ASSERT_EQ(parsedArgs.value().selectedColumns.size(), 3);
        EXPECT_EQ(parsedArgs.value().selectedColumns[0], "pid");
        EXPECT_EQ(parsedArgs.value().selectedColumns[1], "name");
        EXPECT_EQ(parsedArgs.value().selectedColumns[2], "cpu");
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithNoTruncateCmdline) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--no-truncate-cmdline"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs.value().command, "list");
        EXPECT_TRUE(parsedArgs.value().noTruncateCmdline);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithOutput) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--output", TEST_OUTPUT_JSON};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->outputFormat.has_value());
        EXPECT_EQ(*parsedArgs->outputFormat, TEST_OUTPUT_JSON);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithVerticalOutput) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", std::to_string(TEST_PID), "--output", TEST_OUTPUT_VERTICAL};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "show");
        ASSERT_TRUE(parsedArgs->outputFormat.has_value());
        EXPECT_EQ(*parsedArgs->outputFormat, TEST_OUTPUT_VERTICAL);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithStateFilter) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--state", std::string(1, TEST_STATE_R)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->stateFilter.has_value());
        EXPECT_EQ(*parsedArgs->stateFilter, TEST_STATE_R);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithSortByAscending) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--sort-by", "pid"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->sortBy.has_value());
        EXPECT_EQ(*parsedArgs->sortBy, ProcessSortField::pid);
        EXPECT_EQ(parsedArgs->sortOrder, SortOrder::asc); // Default
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithSortByDescending) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--sort-by", "cpu", "--sort-order", "desc"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->sortBy.has_value());
        EXPECT_EQ(*parsedArgs->sortBy, ProcessSortField::cpuUsage);
        EXPECT_EQ(parsedArgs->sortOrder, SortOrder::desc);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithHelp) {
    std::vector<std::string> args = {"processAnalyzer", "--help"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_TRUE(parsedArgs.value().showHelp);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithChildren) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", std::to_string(TEST_PID_GENERIC), "--children"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs.value().command, "show");
        EXPECT_TRUE(parsedArgs.value().showChildren);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithOpenFiles) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", std::to_string(TEST_PID_GENERIC), "--open-files"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs.value().command, "show");
        EXPECT_TRUE(parsedArgs.value().showOpenFiles);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithThreads) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", std::to_string(TEST_PID_GENERIC), "--threads"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs.value().command, "show");
        EXPECT_TRUE(parsedArgs.value().showThreads);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithNetwork) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", std::to_string(TEST_PID_GENERIC), "--network"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs.value().command, "show");
        EXPECT_TRUE(parsedArgs.value().showNetworkConnections);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithPpidFilter) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--ppid", std::to_string(TEST_PPID)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->ppidFilter.has_value());
        EXPECT_EQ(*parsedArgs->ppidFilter, TEST_PPID);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithConfigFile) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--config-file", TEST_CONFIG_FILE_DEFAULT};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->configFilePath.has_value());
        EXPECT_EQ(*parsedArgs->configFilePath, TEST_CONFIG_FILE_DEFAULT);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
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

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs.value().command, "list");
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineMultipleOptions) {
    std::vector<std::string> args = {
        "processAnalyzer", "list",
        "--name", TEST_NAME_CHROME,
        "-u", TEST_USER_USER1,
        "-s", std::string(1, TEST_STATE_S),
        "--sort-by", "mem", "--sort-order", "desc",
        "--columns", "pid,name,cmdline",
        "--brief",
        "--no-truncate-cmdline",
        "--output", TEST_OUTPUT_CSV,
        "--ppid", std::to_string(TEST_PPID_MULTIPLE_OPTIONS),
        "--config-file", TEST_CONFIG_FILE_CUSTOM
    };
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->name.has_value());
        EXPECT_EQ(*parsedArgs->name, TEST_NAME_CHROME);
        ASSERT_TRUE(parsedArgs->user.has_value());
        EXPECT_EQ(*parsedArgs->user, TEST_USER_USER1);
        ASSERT_TRUE(parsedArgs->stateFilter.has_value());
        EXPECT_EQ(*parsedArgs->stateFilter, TEST_STATE_S);
        ASSERT_TRUE(parsedArgs->sortBy.has_value());
        // Note: "mem" argument maps to ProcessSortField::memoryPercentage
        ASSERT_TRUE(parsedArgs->sortBy.has_value()); EXPECT_EQ(*parsedArgs->sortBy, ProcessSortField::memoryPercentage);
        EXPECT_EQ(parsedArgs->sortOrder, SortOrder::desc);
        ASSERT_EQ(parsedArgs->selectedColumns.size(), 3);
        EXPECT_EQ(parsedArgs->selectedColumns[0], "pid");
        EXPECT_EQ(parsedArgs->selectedColumns[1], "name");
        EXPECT_EQ(parsedArgs->selectedColumns[2], "cmdline");
        EXPECT_TRUE(parsedArgs->briefMode);
        EXPECT_TRUE(parsedArgs->noTruncateCmdline);
        ASSERT_TRUE(parsedArgs->outputFormat.has_value());
        EXPECT_EQ(parsedArgs->outputFormat, TEST_OUTPUT_CSV);
        ASSERT_TRUE(parsedArgs->ppidFilter.has_value());
        EXPECT_EQ(parsedArgs->ppidFilter, TEST_PPID_MULTIPLE_OPTIONS);
        ASSERT_TRUE(parsedArgs->configFilePath.has_value());
        EXPECT_EQ(parsedArgs->configFilePath, TEST_CONFIG_FILE_CUSTOM);
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

TEST_F(ArgsTestFixture, ParseCommandLinePositionalPidCommand) {
    std::vector<std::string> args = {"processAnalyzer", "pid", std::to_string(TEST_PID), "--threads"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs.value().command, "pid");
        ASSERT_TRUE(parsedArgs.value().pid.has_value());
        EXPECT_EQ(*parsedArgs->pid, TEST_PID);
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
    std::vector<std::string> args = {"processAnalyzer", "name", TEST_NAME_FIREFOX};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "name");
    ASSERT_TRUE(parsedArgs->name.has_value());
    EXPECT_EQ(*parsedArgs->name, TEST_NAME_FIREFOX);
}

TEST_F(ArgsTestFixture, ParseCommandLinePositionalUserCommand) {
    std::vector<std::string> args = {"processAnalyzer", "user", TEST_USER_ROOT};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs.value().command, "user");
        ASSERT_TRUE(parsedArgs.value().user.has_value());
        EXPECT_EQ(*parsedArgs->user, TEST_USER_ROOT);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLinePidCommandRejectsPpidFilter) {
    std::vector<std::string> args = {"processAnalyzer", "pid", std::to_string(TEST_PID_REJECT), "--ppid", std::to_string(TEST_PPID_REJECT)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineRejectsOutOfRangeLongPid) {
    // Using a value that exceeds typical int limits for PID.
    // The exact limit depends on the system, but this should be large enough
    // to test potential overflow or validation issues.
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", MAX_LL_STR}; // Max long long
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineRejectsOutOfRangePositionalPid) {
    // Using a value that exceeds typical int limits for PID.
    std::vector<std::string> args = {"processAnalyzer", "pid", MAX_LL_STR}; // Max long long
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineRejectsOutOfRangePpid) {
    // Using a value that exceeds typical int limits for PPID.
    std::vector<std::string> args = {"processAnalyzer", "list", "--ppid", MIN_LL_STR}; // Min long long
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}
