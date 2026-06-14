// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "cli/output.h"
#include "cli/args.h"
#include "analyzer/process_model.h"

#include <string>
#include <vector>
#include <span>
#include <optional>

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

TEST(OutputTest, VerticalDetailsPrintsCpuTime) {
    constexpr unsigned long long kUserTicks = 4200;
    constexpr unsigned long long kKernelTicks = 1800;
    ProcessInfo info = makeProcess();
    info.cpuUserTimeTicks = kUserTicks;
    info.cpuKernelTimeTicks = kKernelTicks;

    testing::internal::CaptureStdout();
    printVerticalProcessDetails(info);
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("CPU User Time:"), std::string::npos);
    EXPECT_NE(out.find("CPU Kernel Time:"), std::string::npos);
    EXPECT_NE(out.find("4200"), std::string::npos);
    EXPECT_NE(out.find("1800"), std::string::npos);
}

TEST(OutputTest, DefaultColumnsFullDetails) {
    const auto cols = getDefaultColumnsForTable(true);
    const std::vector<std::string> expected = {
        "pid", "user", "name", "state", "rss", "vm", "threads", "cmdline"};
    EXPECT_EQ(cols, expected);
}

TEST(OutputTest, DefaultColumnsBrief) {
    const auto cols = getDefaultColumnsForTable(false);
    const std::vector<std::string> expected = {"pid", "user", "name", "state", "rss"};
    EXPECT_EQ(cols, expected);
}

TEST(JsonEscapeTest, PrintableAsciiPassesThrough) {
    EXPECT_EQ(jsonEscape("hello world/123"), "hello world/123");
    EXPECT_EQ(jsonEscape(""), "");
}

TEST(JsonEscapeTest, QuoteAndBackslashEscaped) {
    EXPECT_EQ(jsonEscape("a\"b"), "a\\\"b");
    EXPECT_EQ(jsonEscape("a\\b"), "a\\\\b");
}

TEST(JsonEscapeTest, ShortControlEscapes) {
    EXPECT_EQ(jsonEscape("\b\f\n\r\t"), "\\b\\f\\n\\r\\t");
}

TEST(JsonEscapeTest, LowControlBytesUseUnicodeEscape) {
    EXPECT_EQ(jsonEscape(std::string(1, '\x01')), "\\u0001");
    EXPECT_EQ(jsonEscape(std::string(1, '\x1f')), "\\u001f");
}

namespace {
ProcessInfo makeNode(pid_t pid, pid_t ppid, std::string name) {
    ProcessInfo info;
    info.pid = pid;
    info.ppid = ppid;
    info.name = std::move(name);
    return info;
}
} // namespace

// The forest view nests children under their parent by ppid, indenting each
// level by two spaces, so a parent/child/grandchild chain renders as a tree.
TEST(OutputTest, ForestNestsChildrenUnderParents) {
    const std::vector<ProcessInfo> procs = {
        makeNode(100, 1, "init-child"),
        makeNode(200, 100, "child"),
        makeNode(300, 200, "grandchild"),
    };
    testing::internal::CaptureStdout();
    printProcessForest(procs, /*noTruncateCmdline=*/false);
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_EQ(out, "100 init-child\n  200 child\n    300 grandchild\n");
}

// A process whose ppid is not in the displayed set roots its own subtree, and a
// ppid cycle is still printed (never silently dropped) and terminates.
TEST(OutputTest, ForestPrintsCycleNodesAndMultipleRoots) {
    const std::vector<ProcessInfo> procs = {
        makeNode(10, 999, "rootA"),   // parent 999 not present -> root
        makeNode(20, 999, "rootB"),   // parent 999 not present -> root
        makeNode(30, 31, "cycleA"),   // 30 <-> 31 form a cycle
        makeNode(31, 30, "cycleB"),
    };
    testing::internal::CaptureStdout();
    printProcessForest(procs, /*noTruncateCmdline=*/false);
    const std::string out = testing::internal::GetCapturedStdout();

    // Every pid appears exactly once.
    for (const char* pid : {"10 rootA", "20 rootB", "30 cycleA", "31 cycleB"}) {
        EXPECT_NE(out.find(pid), std::string::npos);
    }
    // Both standalone roots print at column zero.
    EXPECT_NE(out.find("\n20 rootB"), std::string::npos);
}

// With --no-truncate-cmdline the forest appends each process's full command
// line after its name.
TEST(OutputTest, ForestShowsCmdlineWhenNoTruncate) {
    constexpr pid_t kForestPid = 100;
    constexpr pid_t kForestPpid = 1;
    ProcessInfo proc = makeNode(kForestPid, kForestPpid, "bash");
    proc.cmdline = "bash -c 'sleep 1000'";
    testing::internal::CaptureStdout();
    printProcessForest({proc}, /*noTruncateCmdline=*/true);
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_NE(out.find("100 bash"), std::string::npos);
    EXPECT_NE(out.find("bash -c 'sleep 1000'"), std::string::npos);
}

// An empty process set renders nothing rather than crashing.
TEST(OutputTest, ForestEmptyInputProducesNoOutput) {
    testing::internal::CaptureStdout();
    printProcessForest({}, /*noTruncateCmdline=*/false);
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(out.empty());
}

// NDJSON emits one compact JSON object per process per line with no enclosing
// array, so streaming consumers can parse it line by line.
TEST(OutputTest, NdjsonEmitsOneObjectPerLineNoArray) {
    const std::vector<ProcessInfo> procs = {
        makeNode(100, 1, "alpha"),
        makeNode(200, 1, "beta"),
    };
    testing::internal::CaptureStdout();
    printProcessNdjson(procs, {"pid", "name"});
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_EQ(out.find('['), std::string::npos);          // no enclosing array
    ASSERT_FALSE(out.empty());
    EXPECT_EQ(out.front(), '{');                           // first line is an object
    EXPECT_EQ(out.back(), '\n');                           // trailing newline
    EXPECT_NE(out.find("}\n{"), std::string::npos);        // two objects, one per line
    EXPECT_NE(out.find("\"name\": \"alpha\""), std::string::npos);
    EXPECT_NE(out.find("\"name\": \"beta\""), std::string::npos);
}

// --- OutputFieldTest: raw single-field scalar output (--field NAME) ---

namespace {
// Parse a command line from string args, mirroring main()'s entry contract.
// Mutable copies guard against parseCommandLine touching argv contents.
std::optional<ParsedArguments> parseArgsForField(const std::vector<std::string>& args) {
    std::vector<std::vector<char>> bufs;
    bufs.reserve(args.size());
    std::vector<char*> argv;
    argv.reserve(args.size());
    for (const auto& a : args) {
        bufs.emplace_back(a.begin(), a.end());
        bufs.back().push_back('\0');
        argv.push_back(bufs.back().data());
    }
    return parseCommandLine(static_cast<int>(argv.size()),
                            std::span<char* const>(argv.data(), argv.size()));
}
} // namespace

// A numeric field prints one raw value per process per line, nothing else.
TEST(OutputFieldTest, EmitsOneRawValuePerLine) {
    const std::vector<ProcessInfo> procs = {
        makeNode(100, 1, "alpha"),
        makeNode(200, 1, "beta"),
    };
    testing::internal::CaptureStdout();
    printProcessField(procs, "pid");
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_EQ(out, "100\n200\n");
}

// A string field is emitted verbatim — no quoting, braces, or delimiters — so
// it pipes cleanly into a shell loop.
TEST(OutputFieldTest, EmitsStringFieldWithoutQuotesOrBraces) {
    const std::vector<ProcessInfo> procs = {
        makeNode(100, 1, "alpha"),
        makeNode(200, 1, "beta"),
    };
    testing::internal::CaptureStdout();
    printProcessField(procs, "name");
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_EQ(out, "alpha\nbeta\n");
    EXPECT_EQ(out.find('"'), std::string::npos);
    EXPECT_EQ(out.find('{'), std::string::npos);
    EXPECT_EQ(out.find(','), std::string::npos);
}

// An empty process set emits nothing (not even a "no processes" notice), so an
// empty result is an empty stream for the consumer.
TEST(OutputFieldTest, EmptySetEmitsNothing) {
    testing::internal::CaptureStdout();
    printProcessField({}, "pid");
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_TRUE(out.empty());
}

// --field accepts a known field name and rejects an unknown one (or a missing
// argument) at parse time, so getProcessInfoValue's std::unreachable is safe.
TEST(OutputFieldTest, ParseAcceptsValidFieldRejectsUnknown) {
    auto ok = parseArgsForField({"processAnalyzer", "list", "--field", "pid"});
    ASSERT_TRUE(ok.has_value());
    ASSERT_TRUE(ok->singleField.has_value());
    EXPECT_EQ(*ok->singleField, "pid");

    EXPECT_FALSE(parseArgsForField({"processAnalyzer", "list", "--field", "bogus"}).has_value());
    EXPECT_FALSE(parseArgsForField({"processAnalyzer", "list", "--field"}).has_value());
}

// Raw scalar mode is deliberately unescaped (unlike json/csv, which escape
// control characters): the bytes pass through verbatim so the consumer sees the
// real value. This pins that contract.
TEST(OutputFieldTest, EmitsRawUnescapedValues) {
    ProcessInfo p = makeNode(1, 0, "a\tb");
    testing::internal::CaptureStdout();
    printProcessField({p}, "name");
    const std::string out = testing::internal::GetCapturedStdout();

    EXPECT_EQ(out, "a\tb\n");
    EXPECT_NE(out.find('\t'), std::string::npos);   // a raw tab byte, not "\\t"
    EXPECT_EQ(out.find("\\t"), std::string::npos);  // never the JSON-style escape
}

// --field threads through the show/pid detail commands, not only list.
TEST(OutputFieldTest, ParsesFieldForShowCommand) {
    auto parsed = parseArgsForField({"processAnalyzer", "show", "--pid", "1234", "--field", "name"});
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->command, "show");
    ASSERT_TRUE(parsed->pid.has_value());
    EXPECT_EQ(*parsed->pid, 1234);
    ASSERT_TRUE(parsed->singleField.has_value());
    EXPECT_EQ(*parsed->singleField, "name");
}

// A representative sample of field types — int, string, long long, long — all
// parse, so the whole column vocabulary is reachable via --field.
TEST(OutputFieldTest, AcceptsDiverseFieldTypes) {
    for (const std::string field : {"ppid", "user", "rss", "threads", "vm", "nice"}) {
        auto parsed = parseArgsForField({"processAnalyzer", "list", "--field", field});
        ASSERT_TRUE(parsed.has_value()) << field;
        ASSERT_TRUE(parsed->singleField.has_value()) << field;
        EXPECT_EQ(*parsed->singleField, field);
    }
}
