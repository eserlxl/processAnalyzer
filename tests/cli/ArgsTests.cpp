// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "cli/Args.h"
#include <vector>
#include <string>
#include <optional>

// Helper to convert std::vector<std::string> to std::vector<char*> for argv
std::vector<char*> makeArgv(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (const std::string& arg : args) {
        argv.push_back(const_cast<char*>(arg.c_str()));
    }
    return argv;
}

TEST(ArgsTests, ParseCommandLineBasic) {
    std::vector<std::string> args = {"processAnalyzer", "list"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    EXPECT_FALSE(parsedArgs->pid.has_value());
    EXPECT_FALSE(parsedArgs->name.has_value());
    EXPECT_FALSE(parsedArgs->user.has_value());
    EXPECT_FALSE(parsedArgs->briefMode);
    EXPECT_TRUE(parsedArgs->selectedColumns.empty());
}

TEST(ArgsTests, ParseCommandLineWithPid) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "1234"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "show");
    ASSERT_TRUE(parsedArgs->pid.has_value());
    EXPECT_EQ(parsedArgs->pid.value(), 1234);
}

TEST(ArgsTests, ParseCommandLineWithPidShort) {
    std::vector<std::string> args = {"processAnalyzer", "show", "-p", "1234"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "show");
    ASSERT_TRUE(parsedArgs->pid.has_value());
    EXPECT_EQ(parsedArgs->pid.value(), 1234);
}

TEST(ArgsTests, ParseCommandLineWithName) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--name", "firefox"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    ASSERT_TRUE(parsedArgs->name.has_value());
    EXPECT_EQ(parsedArgs->name.value(), "firefox");
}

TEST(ArgsTests, ParseCommandLineWithUser) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--user", "root"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    ASSERT_TRUE(parsedArgs->user.has_value());
    EXPECT_EQ(parsedArgs->user.value(), "root");
}

TEST(ArgsTests, ParseCommandLineWithBrief) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--brief"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    EXPECT_TRUE(parsedArgs->briefMode);
}

TEST(ArgsTests, ParseCommandLineWithColumns) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--columns", "pid,name,cpu"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    ASSERT_EQ(parsedArgs->selectedColumns.size(), 3);
    EXPECT_EQ(parsedArgs->selectedColumns[0], "pid");
    EXPECT_EQ(parsedArgs->selectedColumns[1], "name");
    EXPECT_EQ(parsedArgs->selectedColumns[2], "cpu");
}

TEST(ArgsTests, ParseCommandLineWithNoTruncateCmdline) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--no-truncate-cmdline"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    EXPECT_TRUE(parsedArgs->noTruncateCmdline);
}

TEST(ArgsTests, ParseCommandLineWithOutput) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--output", "json"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    ASSERT_TRUE(parsedArgs->outputFormat.has_value());
    EXPECT_EQ(parsedArgs->outputFormat.value(), "json");
}

TEST(ArgsTests, ParseCommandLineWithStateFilter) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--state", "R"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    ASSERT_TRUE(parsedArgs->stateFilter.has_value());
    EXPECT_EQ(parsedArgs->stateFilter.value(), 'R');
}

TEST(ArgsTests, ParseCommandLineWithSortByAscending) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--sort-by", "pid"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    ASSERT_TRUE(parsedArgs->sortBy.has_value());
    EXPECT_EQ(parsedArgs->sortBy.value(), ProcessSortField::pid);
    EXPECT_EQ(parsedArgs->sortOrder, SortOrder::asc); // Default
}

TEST(ArgsTests, ParseCommandLineWithSortByDescending) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--sort-by", "cpu", "--sort-order", "desc"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    ASSERT_TRUE(parsedArgs->sortBy.has_value());
    EXPECT_EQ(parsedArgs->sortBy.value(), ProcessSortField::cpuUsage);
    EXPECT_EQ(parsedArgs->sortOrder, SortOrder::desc);
}

TEST(ArgsTests, ParseCommandLineWithHelp) {
    std::vector<std::string> args = {"processAnalyzer", "--help"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_TRUE(parsedArgs->showHelp);
}

TEST(ArgsTests, ParseCommandLineWithChildren) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "123", "--children"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "show");
    EXPECT_TRUE(parsedArgs->showChildren);
}

TEST(ArgsTests, ParseCommandLineWithOpenFiles) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "123", "--open-files"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "show");
    EXPECT_TRUE(parsedArgs->showOpenFiles);
}

TEST(ArgsTests, ParseCommandLineWithThreads) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "123", "--threads"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "show");
    EXPECT_TRUE(parsedArgs->showThreads);
}

TEST(ArgsTests, ParseCommandLineWithNetwork) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "123", "--network"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "show");
    EXPECT_TRUE(parsedArgs->showNetworkConnections);
}

TEST(ArgsTests, ParseCommandLineWithPpidFilter) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--ppid", "5678"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    ASSERT_TRUE(parsedArgs->ppidFilter.has_value());
    EXPECT_EQ(parsedArgs->ppidFilter.value(), 5678);
}

TEST(ArgsTests, ParseCommandLineWithConfigFile) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--config-file", "/etc/processAnalyzer.conf"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    ASSERT_TRUE(parsedArgs->configFilePath.has_value());
    EXPECT_EQ(parsedArgs->configFilePath.value(), "/etc/processAnalyzer.conf");
}

TEST(ArgsTests, ParseCommandLineInvalidPid) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid", "abc"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST(ArgsTests, ParseCommandLineMissingPidValue) {
    std::vector<std::string> args = {"processAnalyzer", "show", "--pid"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST(ArgsTests, ParseCommandLineInvalidSortBy) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--sort-by", "invalid"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST(ArgsTests, ParseCommandLineInvalidSortOrder) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--sort-by", "pid", "--sort-order", "invalid"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST(ArgsTests, ParseCommandLineInvalidState) {
    std::vector<std::string> args = {"processAnalyzer", "list", "--state", "RUNNING"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST(ArgsTests, ParseCommandLineUnknownCommand) {
    std::vector<std::string> args = {"processAnalyzer", "unknown_command"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_FALSE(parsedArgs.has_value());
}

TEST(ArgsTests, ParseCommandLineNoCommand) {
    std::vector<std::string> args = {"processAnalyzer"};
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
}

TEST(ArgsTests, ParseCommandLineMultipleOptions) {
    std::vector<std::string> args = {
        "processAnalyzer", "list",
        "--name", "chrome",
        "-u", "user1",
        "-s", "S",
        "--sort-by", "mem", "--sort-order", "desc",
        "--columns", "pid,name,cmd",
        "--brief",
        "--no-truncate-cmdline",
        "--output", "csv",
        "--ppid", "1000",
        "--config-file", "my_config.ini"
    };
    std::vector<char*> argv = makeArgv(args);
    std::optional<ParsedArguments> parsedArgs = parseCommandLine(static_cast<int>(argv.size()), argv);

    ASSERT_TRUE(parsedArgs.has_value());
    EXPECT_EQ(parsedArgs->command, "list");
    ASSERT_TRUE(parsedArgs->name.has_value());
    EXPECT_EQ(parsedArgs->name.value(), "chrome");
    ASSERT_TRUE(parsedArgs->user.has_value());
    EXPECT_EQ(parsedArgs->user.value(), "user1");
    ASSERT_TRUE(parsedArgs->stateFilter.has_value());
    EXPECT_EQ(parsedArgs->stateFilter.value(), 'S');
    ASSERT_TRUE(parsedArgs->sortBy.has_value());
    EXPECT_EQ(parsedArgs->sortBy.value(), ProcessSortField::memoryPercentage);
    EXPECT_EQ(parsedArgs->sortOrder, SortOrder::desc);
    ASSERT_EQ(parsedArgs->selectedColumns.size(), 3);
    EXPECT_EQ(parsedArgs->selectedColumns[0], "pid");
    EXPECT_EQ(parsedArgs->selectedColumns[1], "name");
    EXPECT_EQ(parsedArgs->selectedColumns[2], "cmd");
    EXPECT_TRUE(parsedArgs->briefMode);
    EXPECT_TRUE(parsedArgs->noTruncateCmdline);
    ASSERT_TRUE(parsedArgs->outputFormat.has_value());
    EXPECT_EQ(parsedArgs->outputFormat.value(), "csv");
    ASSERT_TRUE(parsedArgs->ppidFilter.has_value());
    EXPECT_EQ(parsedArgs->ppidFilter.value(), 1000);
    ASSERT_TRUE(parsedArgs->configFilePath.has_value());
    EXPECT_EQ(parsedArgs->configFilePath.value(), "my_config.ini");
}
