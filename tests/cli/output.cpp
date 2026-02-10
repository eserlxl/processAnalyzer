// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "cli/output.h"
#include "analyzer/process_model.h"
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <chrono>

// Helper to redirect stdout
class StdOutRedirect {
public:
    StdOutRedirect() : oldCoutBuffer(std::cout.rdbuf()) {
        std::cout.rdbuf(ss.rdbuf());
    }

    ~StdOutRedirect() {
        std::cout.rdbuf(oldCoutBuffer);
    }

    std::string getString() {
        return ss.str();
    }

private:
    std::stringstream ss;
    std::streambuf* oldCoutBuffer;
};

// Dummy ProcessInfo for testing
ProcessInfo createDummyProcess(pid_t pid, const std::string& name, const std::string& cmdline,
                               const std::string& username, char state, long long uptimeMs) {
    ProcessInfo p;
    p.pid = pid;
    p.name = name;
    p.cmdline = cmdline;
    p.username = username;
    p.state = state;
    p.residentMemory = 1000 + (pid * 100 % 5000); // in KB
    p.virtualMemory = 10000 + (pid * 100 % 10000); // in KB
    p.threadCount = 1 + (pid % 5);
    p.startTimeUnix = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::milliseconds(uptimeMs));
    p.ppid = (pid > 1) ? pid -1 : 0;
    p.ioReadBytes = 500 + (pid % 100);
    p.ioWriteBytes = 200 + (pid % 50);
    p.cpuUserTimeTicks = 1000 + (pid % 100);
    p.cpuKernelTimeTicks = 500 + (pid % 50);
    p.priority = 0;
    p.cpuUsage = 1.0F + (pid % 10);
    p.memoryPercentage = 0.5F + ((pid % 20) / 100.0F);
    p.environmentVariables = {"PATH=/usr/bin", "LANG=en_US.UTF-8"};
    // ProcessInfo does not directly contain openFiles or networkConnections as vectors of strings/pairs.
    // They are handled by separate structs (OpenFileDescriptorInfo, NetworkConnection) and returned by specific functions.
    return p;
}


TEST(OutputTests, GetDefaultColumnsForTable) {
    std::vector<std::string> briefColumns = getDefaultColumnsForTable(false);
    ASSERT_FALSE(briefColumns.empty());
    // Check for some expected brief columns
    EXPECT_TRUE(std::ranges::find(briefColumns, "pid") != briefColumns.end());
    EXPECT_TRUE(std::ranges::find(briefColumns, "name") != briefColumns.end());

    std::vector<std::string> fullColumns = getDefaultColumnsForTable(true);
    ASSERT_FALSE(fullColumns.empty());
    // Check for some expected full columns, which should be more than brief
    EXPECT_GT(fullColumns.size(), briefColumns.size());
    EXPECT_TRUE(std::ranges::find(fullColumns, "cmdline") != fullColumns.end());
    // CPU% is now "cpu" or "cpu(%)" in header, but the key is "cpu" (or similar, checking impl). 
    // In Output.cpp getProcessInfoValue handles "cpu".
    EXPECT_TRUE(std::ranges::find(fullColumns, "rss") != fullColumns.end());
}

TEST(OutputTests, PrintProcessTableBasic) {
    std::vector<ProcessInfo> processes;
    processes.push_back(createDummyProcess(1, "systemd", "/sbin/init", "root", 'S', 10000000));
    processes.push_back(createDummyProcess(100, "bash", "/bin/bash", "user", 'R', 100000));

    std::vector<std::string> columns = {"pid", "name", "user"};

    StdOutRedirect redirect;
    printProcessTable(processes, columns, false);
    std::string output = redirect.getString();

    // Basic checks for table format
    EXPECT_NE(output.find("PID"), std::string::npos); // Headers are uppercased
    EXPECT_NE(output.find("NAME"), std::string::npos);
    EXPECT_NE(output.find("USER"), std::string::npos);
    
    // Check for content presence. Spacing might vary based on column widths.
    EXPECT_NE(output.find('1'), std::string::npos);
    EXPECT_NE(output.find("systemd"), std::string::npos);
    EXPECT_NE(output.find("root"), std::string::npos);
    
    EXPECT_NE(output.find("100"), std::string::npos);
    EXPECT_NE(output.find("bash"), std::string::npos);
    EXPECT_NE(output.find("user"), std::string::npos);
}

TEST(OutputTests, PrintProcessTableNoTruncate) {
    std::vector<ProcessInfo> processes;
    processes.push_back(createDummyProcess(1, "long-name-process", "/usr/bin/long/path/to/process --arg1 --arg2-with-long-value", "user1", 'R', 100000));
    std::vector<std::string> columns = {"pid", "cmdline"};

    StdOutRedirect redirect;
    printProcessTable(processes, columns, true); // noTruncateCmdline = true
    std::string output = redirect.getString();

    EXPECT_NE(output.find("/usr/bin/long/path/to/process --arg1 --arg2-with-long-value"), std::string::npos);
}

TEST(OutputTests, PrintProcessCsv) {
    std::vector<ProcessInfo> processes;
    processes.push_back(createDummyProcess(1, "systemd", "/sbin/init", "root", 'S', 10000000));
    processes.push_back(createDummyProcess(100, "bash", "/bin/bash", "user", 'R', 100000));

    std::vector<std::string> columns = {"pid", "name", "user"};

    StdOutRedirect redirect;
    printProcessCsv(processes, columns);
    std::string output = redirect.getString();

    EXPECT_NE(output.find("pid,name,user"), std::string::npos);
    EXPECT_NE(output.find("1,systemd,root"), std::string::npos);
    EXPECT_NE(output.find("100,bash,user"), std::string::npos);
}

TEST(OutputTests, PrintProcessJson) {
    std::vector<ProcessInfo> processes;
    processes.push_back(createDummyProcess(1, "systemd", "/sbin/init", "root", 'S', 10000000));
    processes.push_back(createDummyProcess(100, "bash", "/bin/bash", "user", 'R', 100000));

    std::vector<std::string> columns = {"pid", "name", "user"};

    StdOutRedirect redirect;
    printProcessJson(processes, columns);
    std::string output = redirect.getString();

    // Check for JSON structure and content
    EXPECT_EQ(output.front(), '[');
    EXPECT_EQ(output.back(), '\n'); // Newline at the end
    
    // Check content of first object
    EXPECT_NE(output.find("\"pid\": 1"), std::string::npos);
    EXPECT_NE(output.find("\"name\": \"systemd\""), std::string::npos);
    EXPECT_NE(output.find("\"user\": \"root\""), std::string::npos);
    
    // Check content of second object
    EXPECT_NE(output.find("\"pid\": 100"), std::string::npos);
    EXPECT_NE(output.find("\"name\": \"bash\""), std::string::npos);
    EXPECT_NE(output.find("\"user\": \"user\""), std::string::npos);
    
    // More robust checks for JSON structure
    EXPECT_TRUE(output.find("  {") != std::string::npos); // Check for indented object start
    EXPECT_TRUE(output.find('}') != std::string::npos); // Check for closing brace
    EXPECT_TRUE(output.find("},\
") != std::string::npos); // Check for comma between objects
    EXPECT_TRUE(output.find("]\n") != std::string::npos); // Check for closing array bracket
}

TEST(OutputTests, PrintVerticalProcessDetails) {
    ProcessInfo p = createDummyProcess(1234, "test_process", "/usr/bin/test_process --config /etc/test.conf", "testuser", 'S', 500000);

    StdOutRedirect redirect;
    printVerticalProcessDetails(p);
    std::string output = redirect.getString();

    EXPECT_NE(output.find("PID:"), std::string::npos);
    EXPECT_NE(output.find("Name:"), std::string::npos); // Adjusted case
    EXPECT_NE(output.find("Command:"), std::string::npos); // Adjusted key/case
    EXPECT_NE(output.find("User:"), std::string::npos); // Adjusted case
    EXPECT_NE(output.find("State:"), std::string::npos); // Adjusted case
    // EXPECT_NE(output.find("Environment:"), std::string::npos); // Not printed in vertical details in current impl? Let's check impl.

    EXPECT_NE(output.find("1234"), std::string::npos);
    EXPECT_NE(output.find("test_process"), std::string::npos);
    EXPECT_NE(output.find("/usr/bin/test_process --config /etc/test.conf"), std::string::npos);
    EXPECT_NE(output.find("testuser"), std::string::npos);
    EXPECT_NE(output.find('S'), std::string::npos);
}

TEST(OutputTests, PrintUsage) {
    StdOutRedirect redirect;
    printUsage();
    std::string output = redirect.getString();

    EXPECT_NE(output.find("Usage:"), std::string::npos);
    EXPECT_NE(output.find("processAnalyzer [command] [options]"), std::string::npos);
    EXPECT_NE(output.find("Commands:"), std::string::npos);
    EXPECT_NE(output.find("list"), std::string::npos);
    EXPECT_NE(output.find("show"), std::string::npos);
    EXPECT_NE(output.find("Options:"), std::string::npos);
    EXPECT_NE(output.find("--pid <pid>"), std::string::npos);
    EXPECT_NE(output.find("--help"), std::string::npos);
}

TEST(OutputTests, PrintProcessTableTruncation) {
    std::vector<ProcessInfo> processes;
    std::string longCmd = "this-is-a-very-long-command-line-that-will-surely-exceed-the-default-width-of-forty-characters";
    processes.push_back(createDummyProcess(1, "long-cmd", longCmd, "user1", 'R', 100000));
    std::vector<std::string> columns = {"cmdline"};

    StdOutRedirect redirect;
    printProcessTable(processes, columns, false); // noTruncateCmdline = false
    std::string output = redirect.getString();

    // Default width for cmdline is 40. The output should be 39 chars + '~'.
    std::string expectedTrunc = "this-is-a-very-long-command-line-that-w~";
    EXPECT_NE(output.find(expectedTrunc), std::string::npos);
    EXPECT_EQ(output.find(longCmd), std::string::npos); // The full command should not be present
}

TEST(OutputTests, PrintProcessCsvQuoting) {
    std::vector<ProcessInfo> processes;
    processes.push_back(createDummyProcess(1, "comma,name", "cmd, with, comma", "user", 'S', 10000000));
    processes.push_back(createDummyProcess(2, "quote\"name", "cmd with \"quote\"", "user", 'R', 100000));

    std::vector<std::string> columns = {"pid", "name", "cmdline"};

    StdOutRedirect redirect;
    printProcessCsv(processes, columns);
    std::string output = redirect.getString();

    EXPECT_NE(output.find("pid,name,cmdline"), std::string::npos);
    // Check for quoted fields
    EXPECT_NE(output.find("1,\"comma,name\",\"cmd, with, comma\""), std::string::npos);
    // Check for escaped quotes
    EXPECT_NE(output.find("2,\"quote\"\"name\",\"cmd with \"\"quote\"\"\""), std::string::npos);
}
