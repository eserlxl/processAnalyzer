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
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "1234"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "show");
        ASSERT_TRUE(parsedArgs->pid.has_value());
        EXPECT_EQ(parsedArgs->pid.value(), 1234);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithPidShort) {
    std::vector<std::string> args = {"processAnalyzer", "show", "-p", "1234"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "show");
        ASSERT_TRUE(parsedArgs->pid.has_value());
        EXPECT_EQ(parsedArgs->pid.value(), 1234);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithName) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--name", "firefox"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->name.has_value());
        EXPECT_EQ(parsedArgs->name.value(), "firefox");
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithUser) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--user", "root"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->user.has_value());
        EXPECT_EQ(parsedArgs->user.value(), "root");
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
    std::vector<std::string> args = {"processAnalyzer", "list", "--output", "json"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->outputFormat.has_value());
        EXPECT_EQ(parsedArgs->outputFormat.value(), "json");
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithVerticalOutput) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "1234", "--output", "vertical"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "show");
        ASSERT_TRUE(parsedArgs->outputFormat.has_value());
        EXPECT_EQ(parsedArgs->outputFormat.value(), "vertical");
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithStateFilter) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--state", "R"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->stateFilter.has_value());
        EXPECT_EQ(parsedArgs->stateFilter.value(), 'R');
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
        EXPECT_EQ(parsedArgs->sortBy.value(), ProcessSortField::pid);
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
        EXPECT_EQ(parsedArgs->sortBy.value(), ProcessSortField::cpuUsage);
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
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "123", "--children"};
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
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "123", "--open-files"};
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
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "123", "--threads"};
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
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "123", "--network"};
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
    std::vector<std::string> args = {"processAnalyzer", "list", "--ppid", "5678"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->ppidFilter.has_value());
        EXPECT_EQ(parsedArgs->ppidFilter.value(), 5678);
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLineWithConfigFile) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--config-file", "/etc/processAnalyzer.conf"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->configFilePath.has_value());
        EXPECT_EQ(parsedArgs->configFilePath.value(), "/etc/processAnalyzer.conf");
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
        "--name", "chrome",
        "-u", "user1",
        "-s", "S",
        "--sort-by", "mem", "--sort-order", "desc",
        "--columns", "pid,name,cmdline",
        "--brief",
        "--no-truncate-cmdline",
        "--output", "csv",
        "--ppid", "1000",
        "--config-file", "my_config.ini"
    };
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs->command, "list");
        ASSERT_TRUE(parsedArgs->name.has_value());
        EXPECT_EQ(parsedArgs->name.value(), "chrome");
        ASSERT_TRUE(parsedArgs->user.has_value());
        EXPECT_EQ(parsedArgs->user.value(), "user1");
        ASSERT_TRUE(parsedArgs->stateFilter.has_value());
        EXPECT_EQ(parsedArgs->stateFilter.value(), 'S');
        ASSERT_TRUE(parsedArgs->sortBy.has_value());
        // Note: "mem" argument maps to ProcessSortField::memoryPercentage
        EXPECT_EQ(parsedArgs->sortBy.value(), ProcessSortField::memoryPercentage);
        EXPECT_EQ(parsedArgs->sortOrder, SortOrder::desc);
        ASSERT_EQ(parsedArgs->selectedColumns.size(), 3);
        EXPECT_EQ(parsedArgs->selectedColumns[0], "pid");
        EXPECT_EQ(parsedArgs->selectedColumns[1], "name");
        EXPECT_EQ(parsedArgs->selectedColumns[2], "cmdline");
        EXPECT_TRUE(parsedArgs->briefMode);
        EXPECT_TRUE(parsedArgs->noTruncateCmdline);
        ASSERT_TRUE(parsedArgs->outputFormat.has_value());
        EXPECT_EQ(parsedArgs->outputFormat, "csv");
        ASSERT_TRUE(parsedArgs->ppidFilter.has_value());
        EXPECT_EQ(parsedArgs->ppidFilter, 1000);
        ASSERT_TRUE(parsedArgs->configFilePath.has_value());
        EXPECT_EQ(parsedArgs->configFilePath, "my_config.ini");
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
    std::vector<std::string> args = {"processAnalyzer", "pid", "1234", "--threads"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs.value().command, "pid");
        ASSERT_TRUE(parsedArgs.value().pid.has_value());
        EXPECT_EQ(parsedArgs.value().pid.value(), 1234);
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
    std::vector<std::string> args = {"processAnalyzer", "name", "firefox"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "name");
    ASSERT_TRUE(parsedArgs->name.has_value());
    EXPECT_EQ(parsedArgs->name.value(), "firefox");
}

TEST_F(ArgsTestFixture, ParseCommandLinePositionalUserCommand) {
    std::vector<std::string> args = {"processAnalyzer", "user", "root"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    if (parsedArgs) {
        EXPECT_EQ(parsedArgs.value().command, "user");
        ASSERT_TRUE(parsedArgs.value().user.has_value());
        EXPECT_EQ(parsedArgs.value().user.value(), "root");
    } else {
        FAIL() << "Expected a valid parsed argument object.";
    }
}

TEST_F(ArgsTestFixture, ParseCommandLinePidCommandRejectsPpidFilter) {
    std::vector<std::string> args = {"processAnalyzer", "pid", "42", "--ppid", "1"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineRejectsOutOfRangeLongPid) {
    // Using a value that exceeds typical int limits for PID.
    // The exact limit depends on the system, but this should be large enough
    // to test potential overflow or validation issues.
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "9223372036854775807"}; // Max long long
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineRejectsOutOfRangePositionalPid) {
    // Using a value that exceeds typical int limits for PID.
    std::vector<std::string> args = {"processAnalyzer", "pid", "9223372036854775807"}; // Max long long
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseCommandLineRejectsOutOfRangePpid) {
    // Using a value that exceeds typical int limits for PPID.
    std::vector<std::string> args = {"processAnalyzer", "list", "--ppid", "-9223372036854775807"}; // Min long long
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}
