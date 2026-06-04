// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "cli/output.h"
#include "analyzer/process_model.h"

#include <string>
#include <vector>

namespace {
constexpr pid_t kPid = 1234;
constexpr pid_t kPpid = 1;
constexpr uid_t kUid = 1000;
constexpr long long kRss = 2048;
constexpr long long kVm = 4096;
constexpr long kThreads = 3;

ProcessInfo makeProcess() {
    ProcessInfo info;
    info.pid = kPid;
    info.ppid = kPpid;
    info.uid = kUid;
    info.username = "alice";
    info.name = "bash";
    info.state = "S";
    info.residentMemory = kRss;
    info.virtualMemory = kVm;
    info.threadCount = kThreads;
    info.cmdline = "bash -c true";
    return info;
}
} // namespace

// JSON string values must escape control characters, otherwise the output is
// not valid JSON (RFC 8259 §7).
TEST(OutputTest, JsonEscapesControlCharacters) {
    ProcessInfo info = makeProcess();
    info.name = "a\tb\nc";

    testing::internal::CaptureStdout();
    printProcessJson({info}, {"pid", "name"});
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("\\t"), std::string::npos);
    EXPECT_NE(out.find("\\n"), std::string::npos);
    // No raw control byte leaked into the JSON string.
    EXPECT_EQ(out.find('\t'), std::string::npos);
    EXPECT_EQ(out.find("a\tb"), std::string::npos);
}

TEST(OutputTest, JsonEscapesQuoteAndBackslash) {
    ProcessInfo info = makeProcess();
    info.name = "a\"b\\c";

    testing::internal::CaptureStdout();
    printProcessJson({info}, {"name"});
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("\\\""), std::string::npos); // escaped double-quote
    EXPECT_NE(out.find("\\\\"), std::string::npos); // escaped backslash
}

TEST(OutputTest, JsonEmitsNumericColumnsUnquoted) {
    ProcessInfo info = makeProcess();

    testing::internal::CaptureStdout();
    printProcessJson({info}, {"pid"});
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("\"pid\": 1234"), std::string::npos);
    EXPECT_EQ(out.find("\"pid\": \"1234\""), std::string::npos);
}

TEST(OutputTest, CsvQuotesValuesContainingComma) {
    ProcessInfo info = makeProcess();
    info.cmdline = "a,b";

    testing::internal::CaptureStdout();
    printProcessCsv({info}, {"cmdline"});
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("\"a,b\""), std::string::npos);
}

TEST(OutputTest, TablePrintsUppercasedHeader) {
    ProcessInfo info = makeProcess();

    testing::internal::CaptureStdout();
    printProcessTable({info}, {"pid", "name"}, false);
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("PID"), std::string::npos);
    EXPECT_NE(out.find("NAME"), std::string::npos);
    EXPECT_NE(out.find("bash"), std::string::npos);
}

TEST(OutputTest, CsvQuotesValuesContainingNewline) {
    ProcessInfo info = makeProcess();
    info.cmdline = "a\nb";

    testing::internal::CaptureStdout();
    printProcessCsv({info}, {"cmdline"});
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("\"a\nb\""), std::string::npos);
}

TEST(OutputTest, JsonEmitsArraySeparatorBetweenObjects) {
    ProcessInfo first = makeProcess();
    ProcessInfo second = makeProcess();
    second.pid = first.pid + 1;

    testing::internal::CaptureStdout();
    printProcessJson({first, second}, {"pid"});
    const std::string out = testing::internal::GetCapturedStdout();

    ASSERT_FALSE(out.empty());
    EXPECT_EQ(out.front(), '[');
    EXPECT_NE(out.find(']'), std::string::npos);
    EXPECT_NE(out.find("},\n"), std::string::npos); // separator between objects
}

TEST(OutputTest, VerticalDetailsPrintsLabelledFields) {
    ProcessInfo info = makeProcess();

    testing::internal::CaptureStdout();
    printVerticalProcessDetails(info);
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("PID:"), std::string::npos);
    EXPECT_NE(out.find("bash"), std::string::npos);
}

TEST(OutputTest, TableIncludesCwdColumn) {
    ProcessInfo info = makeProcess();
    info.currentWorkingDirectory = "/home/alice/work";

    testing::internal::CaptureStdout();
    printProcessTable({info}, {"pid", "cwd"}, false);
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("CWD"), std::string::npos);
    EXPECT_NE(out.find("/home/alice/work"), std::string::npos);
}

TEST(OutputTest, VerticalDetailsPrintsWorkingDirectory) {
    ProcessInfo info = makeProcess();
    info.currentWorkingDirectory = "/tmp/mydir";

    testing::internal::CaptureStdout();
    printVerticalProcessDetails(info);
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("Working Directory:"), std::string::npos);
    EXPECT_NE(out.find("/tmp/mydir"), std::string::npos);
}

TEST(OutputTest, VerticalDetailsPrintsIoStats) {
    constexpr long long kIoRead = 123456;
    constexpr long long kIoWrite = 654321;
    ProcessInfo info = makeProcess();
    info.ioReadBytes = kIoRead;
    info.ioWriteBytes = kIoWrite;

    testing::internal::CaptureStdout();
    printVerticalProcessDetails(info);
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("IO Read:"), std::string::npos);
    EXPECT_NE(out.find("IO Write:"), std::string::npos);
    EXPECT_NE(out.find("123456"), std::string::npos);
    EXPECT_NE(out.find("654321"), std::string::npos);
}
