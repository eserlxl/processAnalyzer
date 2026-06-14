// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "cli/args.h" // Includes analyzer/process_model.h indirectly

#include <vector>
#include <string>
#include <optional>
#include <span>
#include <csignal>

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

TEST_F(ArgsTestFixture, ParseSystemWatchWithInterval) {
    constexpr int kInterval = 5;
    std::vector<std::string> args = {"processAnalyzer", "system", "--watch", std::to_string(kInterval)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs.value().command, "system");
    ASSERT_TRUE(parsedArgs.value().watchIntervalSeconds.has_value());
    EXPECT_EQ(parsedArgs.value().watchIntervalSeconds.value(), kInterval);
}

TEST_F(ArgsTestFixture, ParseSystemWatchDefaultsInterval) {
    std::vector<std::string> args = {"processAnalyzer", "system", "--watch"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    ASSERT_TRUE(parsedArgs.value().watchIntervalSeconds.has_value());
    EXPECT_EQ(parsedArgs.value().watchIntervalSeconds.value(),
              ParsedArguments::kDefaultWatchIntervalSeconds);
}

TEST_F(ArgsTestFixture, ParseWatchRejectedOutsideSystem) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--watch"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseTopWithCount) {
    constexpr int kCount = 5;
    std::vector<std::string> args = {"processAnalyzer", "top", "--count", std::to_string(kCount)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs.value().command, "top");
    ASSERT_TRUE(parsedArgs.value().topCount.has_value());
    EXPECT_EQ(parsedArgs.value().topCount.value(), kCount);
    EXPECT_FALSE(parsedArgs.value().topByIo);
}

TEST_F(ArgsTestFixture, ParseTopByIo) {
    std::vector<std::string> args = {"processAnalyzer", "top", "--io"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_TRUE(parsedArgs.value().topByIo);
}

TEST_F(ArgsTestFixture, ParseTopOptionsRejectedOutsideTop) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--io"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseTopCountRejectsNonPositive) {
    std::vector<std::string> args = {"processAnalyzer", "top", "--count", "0"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseTopByMem) {
    std::vector<std::string> args = {"processAnalyzer", "top", "--mem"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_TRUE(parsedArgs.value().topByMem);
    EXPECT_FALSE(parsedArgs.value().topByIo);
}

TEST_F(ArgsTestFixture, ParseTopIoAndMemMutuallyExclusive) {
    std::vector<std::string> args = {"processAnalyzer", "top", "--io", "--mem"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseTopWatchAccepted) {
    constexpr int kInterval = 2;
    std::vector<std::string> args = {"processAnalyzer", "top", "--watch", std::to_string(kInterval)};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs.value().command, "top");
    ASSERT_TRUE(parsedArgs.value().watchIntervalSeconds.has_value());
    EXPECT_EQ(parsedArgs.value().watchIntervalSeconds.value(), kInterval);
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

TEST_F(ArgsTestFixture, NetworkPortFilterForList) {
    auto argv = makeArgv({"processAnalyzer", "list", "--network", "8080"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->networkPortFilter.has_value());
    EXPECT_EQ(*parsed->networkPortFilter, static_cast<uint16_t>(8080));
    EXPECT_FALSE(parsed->showNetworkConnections);
}

TEST_F(ArgsTestFixture, NetworkPortFilterRejectedForShowCommand) {
    auto argv = makeArgv({"processAnalyzer", "show", "--pid", "1", "--network", "8080"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsed.has_value());
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

TEST_F(ArgsTestFixture, SortByUid) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "uid"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::uid);
}

TEST_F(ArgsTestFixture, SortByUser) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "user"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::user);
}

TEST_F(ArgsTestFixture, SortByRss) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "rss"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::rss);
}

TEST_F(ArgsTestFixture, SortByVm) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "vm"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::vmsize);
}

TEST_F(ArgsTestFixture, SortByState) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "state"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::state);
}

TEST_F(ArgsTestFixture, SortByPpid) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "ppid"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::ppid);
}

TEST_F(ArgsTestFixture, SortByThreads) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "threads"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::threads);
}

TEST_F(ArgsTestFixture, SortByStartTime) {
    auto argv = makeArgv({"processAnalyzer", "list", "--sort-by", "start-time"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->sortBy.has_value());
    EXPECT_EQ(parsed->sortBy.value(), ProcessSortField::startTime);
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

TEST_F(ArgsTestFixture, UidFilterNegativeValueReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--uid", "-1"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, CmdlineFilterIsSet) {
    auto argv = makeArgv({"processAnalyzer", "list", "--cmdline", "--config"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->cmdlineFilter.has_value());
    EXPECT_EQ(parsed->cmdlineFilter.value(), "--config");
}

TEST_F(ArgsTestFixture, CmdlineFilterMissingValueReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--cmdline"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, MinVmFilterIsSet) {
    constexpr long long kMinVm = 4096LL;
    auto argv = makeArgv({"processAnalyzer", "list", "--min-vm", "4096"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->minVmKb.has_value());
    EXPECT_EQ(parsed->minVmKb.value(), kMinVm);
}

TEST_F(ArgsTestFixture, MaxVmFilterIsSet) {
    constexpr long long kMaxVm = 524288LL;
    auto argv = makeArgv({"processAnalyzer", "list", "--max-vm", "524288"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->maxVmKb.has_value());
    EXPECT_EQ(parsed->maxVmKb.value(), kMaxVm);
}

TEST_F(ArgsTestFixture, MinPriorityFilterIsSet) {
    constexpr int kMinPriority = 0;
    auto argv = makeArgv({"processAnalyzer", "list", "--min-priority", "0"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->minPriority.has_value());
    EXPECT_EQ(parsed->minPriority.value(), kMinPriority);
}

TEST_F(ArgsTestFixture, MaxPriorityFilterIsSet) {
    constexpr int kMaxPriority = 19;
    auto argv = makeArgv({"processAnalyzer", "list", "--max-priority", "19"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->maxPriority.has_value());
    EXPECT_EQ(parsed->maxPriority.value(), kMaxPriority);
}

TEST_F(ArgsTestFixture, EnvFlagIsSet) {
    auto argv = makeArgv({"processAnalyzer", "show", "--pid", "1", "--env"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_TRUE(parsed->showEnv);
}

TEST_F(ArgsTestFixture, MapsFlagIsSet) {
    auto argv = makeArgv({"processAnalyzer", "show", "--pid", "1", "--maps"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_TRUE(parsed->showMemoryMaps);
}

TEST_F(ArgsTestFixture, EnvFlagWithListCommandReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--env"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, MapsFlagWithListCommandReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--maps"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, ChildrenFlagWithListCommandReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--children"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, ThreadsFlagWithListCommandReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--threads"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, LimitsFlagIsSet) {
    auto argv = makeArgv({"processAnalyzer", "show", "--pid", "1", "--limits"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_TRUE(parsed->showLimits);
}

TEST_F(ArgsTestFixture, CgroupFlagIsSet) {
    auto argv = makeArgv({"processAnalyzer", "show", "--pid", "1", "--cgroup"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_TRUE(parsed->showCgroupInfo);
}

TEST_F(ArgsTestFixture, PerfFlagIsSet) {
    auto argv = makeArgv({"processAnalyzer", "show", "--pid", "1", "--perf"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_TRUE(parsed->showPerf);
    EXPECT_EQ(parsed->perfDurationMs, 200);
}

TEST_F(ArgsTestFixture, PerfFlagWithCustomDuration) {
    auto argv = makeArgv({"processAnalyzer", "show", "--pid", "1", "--perf", "500"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_TRUE(parsed->showPerf);
    EXPECT_EQ(parsed->perfDurationMs, 500);
}

TEST_F(ArgsTestFixture, PerfFlagWithListCommandReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--perf"});
    ASSERT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, TopRejectsNonJsonOutputFormat) {
    auto argv = makeArgv({"processAnalyzer", "top", "--output", "csv"});
    EXPECT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, SystemRejectsNonJsonOutputFormat) {
    auto argv = makeArgv({"processAnalyzer", "system", "--output", "vertical"});
    EXPECT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, TopAcceptsJsonOutputFormat) {
    auto argv = makeArgv({"processAnalyzer", "top", "--output", "json"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->command, "top");
    ASSERT_TRUE(parsed->outputFormat.has_value());
    EXPECT_EQ(parsed->outputFormat.value(), "json");
}

TEST_F(ArgsTestFixture, SystemAcceptsJsonOutputFormat) {
    auto argv = makeArgv({"processAnalyzer", "system", "--output", "json"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->command, "system");
    ASSERT_TRUE(parsed->outputFormat.has_value());
    EXPECT_EQ(parsed->outputFormat.value(), "json");
}

TEST_F(ArgsTestFixture, ParseNameRegexValidPattern) {
    auto argv = makeArgv({"processAnalyzer", "list", "--name-regex", "fire.*"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->nameRegexPattern.has_value());
    EXPECT_EQ(parsed->nameRegexPattern.value(), "fire.*");
}

TEST_F(ArgsTestFixture, ParseCmdlineRegexValidPattern) {
    auto argv = makeArgv({"processAnalyzer", "list", "--cmdline-regex", "^/usr/bin/"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    ASSERT_TRUE(parsed->cmdlineRegexPattern.has_value());
    EXPECT_EQ(parsed->cmdlineRegexPattern.value(), "^/usr/bin/");
}

TEST_F(ArgsTestFixture, ParseNameRegexInvalidPatternReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--name-regex", "["});
    EXPECT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, ParseTopWithNameAndUserFilters) {
    auto argv = makeArgv({"processAnalyzer", "top", "--name", "firefox", "--user", "root"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->command, "top");
    ASSERT_TRUE(parsed->name.has_value());
    EXPECT_EQ(parsed->name.value(), "firefox");
    ASSERT_TRUE(parsed->user.has_value());
    EXPECT_EQ(parsed->user.value(), "root");
}

TEST_F(ArgsTestFixture, ParseShowWithDescendants) {
    auto argv = makeArgv({"processAnalyzer", "show", "--pid", "1", "--descendants"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_TRUE(parsed->showDescendants);
}

TEST_F(ArgsTestFixture, DescendantsFlagWithListCommandReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--descendants"});
    EXPECT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

TEST_F(ArgsTestFixture, ParseShowWithAffinity) {
    auto argv = makeArgv({"processAnalyzer", "show", "--pid", "1", "--affinity"});
    auto parsed = parseCommandLine(static_cast<int>(argv.size()), argv);
    ASSERT_TRUE(parsed.has_value());
    EXPECT_TRUE(parsed->showAffinity);
}

TEST_F(ArgsTestFixture, AffinityFlagWithListCommandReturnsNullopt) {
    auto argv = makeArgv({"processAnalyzer", "list", "--affinity"});
    EXPECT_FALSE(parseCommandLine(static_cast<int>(argv.size()), argv).has_value());
}

// The built-in --help text must document the top command's options so users can
// discover them without reading docs/usage.md; --watch applies to top as well.
TEST(PrintUsageTest, DocumentsTopOptions) {
    testing::internal::CaptureStdout();
    printUsage();
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("--count"), std::string::npos);
    EXPECT_NE(out.find("--io"), std::string::npos);
    EXPECT_NE(out.find("--mem"), std::string::npos);
    // The --watch line must name the top command, not only system.
    const std::string::size_type watchPos = out.find("--watch");
    ASSERT_NE(watchPos, std::string::npos);
    const std::string::size_type lineEnd = out.find('\n', watchPos);
    EXPECT_NE(out.substr(watchPos, lineEnd - watchPos).find("top"), std::string::npos);
}

TEST(PrintUsageTest, DocumentsNdjsonOutputFormat) {
    testing::internal::CaptureStdout();
    printUsage();
    const std::string out = testing::internal::GetCapturedStdout();

    // The --help output must advertise every format the parser accepts, including
    // ndjson and tree, so the help text stays in sync with parseCommandLine.
    EXPECT_NE(out.find("ndjson"), std::string::npos);
    EXPECT_NE(out.find("tree"), std::string::npos);
}

TEST(HasStaticProcessFilterTest, FalseWhenNoFilterSet) {
    ParsedArguments args;
    args.command = "top";
    EXPECT_FALSE(hasStaticProcessFilter(args));
}

TEST(HasStaticProcessFilterTest, TrueForNameRegexPattern) {
    // Regression: top --name-regex populates filter.nameRegex but was never applied
    // because the gate omitted nameRegexPattern.
    ParsedArguments args;
    args.command = "top";
    args.nameRegexPattern = "foo.*";
    EXPECT_TRUE(hasStaticProcessFilter(args));
}

TEST(HasStaticProcessFilterTest, TrueForCmdlineRegexPattern) {
    // Regression: top --cmdline-regex was likewise silently ignored.
    ParsedArguments args;
    args.command = "top";
    args.cmdlineRegexPattern = "bar";
    EXPECT_TRUE(hasStaticProcessFilter(args));
}

TEST(HasStaticProcessFilterTest, TrueForEachStaticCriterion) {
    { ParsedArguments a; a.name = "x"; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.cmdlineFilter = "x"; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.user = "root"; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.stateFilter = 'R'; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.uidFilter = 1; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.ppidFilter = 1; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.minRssKb = 1; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.maxRssKb = 1; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.minVmKb = 1; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.maxVmKb = 1; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.minThreads = 1; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.maxThreads = 1; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.minPriority = 1; EXPECT_TRUE(hasStaticProcessFilter(a)); }
    { ParsedArguments a; a.maxPriority = 1; EXPECT_TRUE(hasStaticProcessFilter(a)); }
}

TEST_F(ArgsTestFixture, ParseSignalCommandWithSignalName) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "signal", "1234", "TERM"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "signal");
    ASSERT_TRUE(parsedArgs->pid.has_value());
    EXPECT_EQ(parsedArgs->pid.value(), testPid);
    ASSERT_TRUE(parsedArgs->signalNumber.has_value());
    EXPECT_EQ(parsedArgs->signalNumber.value(), SIGTERM);
}

TEST_F(ArgsTestFixture, ParseSignalCommandAcceptsSigPrefixAndCase) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "signal", "1234", "sigKILL"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    ASSERT_TRUE(parsedArgs->signalNumber.has_value());
    EXPECT_EQ(parsedArgs->signalNumber.value(), SIGKILL);
}

TEST_F(ArgsTestFixture, ParseSignalCommandWithNumber) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "signal", "1234", "9"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    ASSERT_TRUE(parsedArgs->signalNumber.has_value());
    EXPECT_EQ(parsedArgs->signalNumber.value(), SIGKILL); // signal 9 == SIGKILL
}

TEST_F(ArgsTestFixture, ParseSignalCommandAllowsZeroAsExistenceProbe) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "signal", "1234", "0"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    ASSERT_TRUE(parsedArgs->signalNumber.has_value());
    EXPECT_EQ(parsedArgs->signalNumber.value(), 0);
}

TEST_F(ArgsTestFixture, ParseSignalCommandRejectsInvalidSignal) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "signal", "1234", "NOPE"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseSignalCommandRejectsOutOfRangeNumber) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "signal", "1234", "999"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseSignalCommandRequiresSignal) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "signal", "1234"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseSignalCommandRejectsMissingPid) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "signal"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseSignalCommandRejectsInvalidPid) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "signal", "notapid", "TERM"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, PrintUsageMentionsSignalCommand) {
    testing::internal::CaptureStdout();
    printUsage();
    const std::string out = testing::internal::GetCapturedStdout();
    EXPECT_NE(out.find("signal"), std::string::npos);
}

TEST_F(ArgsTestFixture, ParseReniceCommandWithPositiveNice) {
    constexpr int kNice = 10;
    std::vector<char*> argv = makeArgv({"processAnalyzer", "renice", "1234", "10"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "renice");
    ASSERT_TRUE(parsedArgs->pid.has_value());
    EXPECT_EQ(parsedArgs->pid.value(), testPid);
    ASSERT_TRUE(parsedArgs->niceValue.has_value());
    EXPECT_EQ(parsedArgs->niceValue.value(), kNice);
}

TEST_F(ArgsTestFixture, ParseReniceCommandWithNegativeNice) {
    constexpr int kNice = -20;
    std::vector<char*> argv = makeArgv({"processAnalyzer", "renice", "1234", "-20"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    ASSERT_TRUE(parsedArgs->niceValue.has_value());
    EXPECT_EQ(parsedArgs->niceValue.value(), kNice);
}

TEST_F(ArgsTestFixture, ParseReniceCommandRejectsOutOfRangeNice) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "renice", "1234", "20"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseReniceCommandRejectsMissingNice) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "renice", "1234"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseReniceCommandRejectsInvalidNice) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "renice", "1234", "lots"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseReniceCommandRejectsMissingPid) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "renice"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseAffinityCommandSingleCpu) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "affinity", "1234", "2"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "affinity");
    ASSERT_TRUE(parsedArgs->pid.has_value());
    EXPECT_EQ(parsedArgs->pid.value(), testPid);
    ASSERT_TRUE(parsedArgs->affinityCpus.has_value());
    EXPECT_EQ(parsedArgs->affinityCpus.value(), (std::vector<int>{2}));
}

TEST_F(ArgsTestFixture, ParseAffinityCommandListWithRangeSortedUnique) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "affinity", "1234", "3,0,2-3"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    ASSERT_TRUE(parsedArgs->affinityCpus.has_value());
    // 3,0,2-3 -> {0,2,3} sorted and de-duplicated.
    EXPECT_EQ(parsedArgs->affinityCpus.value(), (std::vector<int>{0, 2, 3}));
}

TEST_F(ArgsTestFixture, ParseAffinityCommandRejectsReversedRange) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "affinity", "1234", "3-1"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseAffinityCommandRejectsInvalidToken) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "affinity", "1234", "0,x"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseAffinityCommandRejectsMissingList) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "affinity", "1234"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}

TEST_F(ArgsTestFixture, ParseAffinityCommandRejectsMissingPid) {
    std::vector<char*> argv = makeArgv({"processAnalyzer", "affinity"});
    std::optional<ParsedArguments> parsedArgs =
        parseCommandLine(static_cast<int>(argv.size()), argv);
    EXPECT_FALSE(parsedArgs.has_value());
}
