// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/test.h" // Assuming MockProc is defined here
#include <sstream>
#include <memory>
#include <optional>
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <ranges>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

// Define constants for common magic numbers
constexpr unsigned long kbInBytes = 1024;
constexpr unsigned long mbInBytes = 1024 * kbInBytes;

// Constants for test values
constexpr unsigned long mapOffset = 0x1000;
constexpr unsigned long inode = 12345;
constexpr unsigned long ioRchar = 1000;
constexpr unsigned long ioWchar = 2000;
constexpr unsigned long ioReadBytes = 500;
constexpr unsigned long ioWriteBytes = 1500;
constexpr unsigned long ioSmallRchar = 100;
constexpr unsigned long ioSmallWchar = 200;
constexpr int pid123 = 123;
constexpr unsigned long utime100 = 100;
constexpr unsigned long stime50 = 50;
constexpr unsigned long vsize = 1234567;
constexpr unsigned long rss1000 = 1000;
constexpr unsigned long utime10 = 10;
constexpr unsigned long stime20 = 20;
constexpr int ppid10 = 10;
constexpr unsigned long utime123 = 123;
constexpr unsigned long stime45 = 45;
constexpr int ppid50 = 50;
constexpr unsigned long ioRchar5000 = 5000;
constexpr unsigned long ioWriteBytes10000 = 10000;
constexpr int fd10 = 10;

// Helper to read file content for verification
std::optional<std::string> readFileContent(const fs::path& filePath) {
    std::ifstream file(filePath, std::ios::binary); // Open in binary to preserve line endings
    if (!file.is_open()) {
        return std::nullopt;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper to split a string by null terminators
std::vector<std::string> splitNullSeparatedStrings(const std::string& content) {
    std::vector<std::string> result;
    if (content.empty()) {
        return result;
    }
    std::string::size_type start = 0;
    std::string::size_type end = content.find('\0');
    while (end != std::string::npos) {
        result.push_back(content.substr(start, end - start));
        start = end + 1;
        end = content.find('\0', start);
    }
    if (start < content.length()) {
        result.push_back(content.substr(start));
    }
    return result;
}

// Helper to read null-terminated strings from a file
std::vector<std::string> readNullSeparatedStrings(const fs::path& filePath) {
    auto contentOpt = readFileContent(filePath);
    if (!contentOpt) {
        return {};
    }
    return splitNullSeparatedStrings(contentOpt.value());
}

} // namespace

// Test fixture for MockProc tests
class MockProcTest : public ::testing::Test {
protected:
    std::unique_ptr<MockProc> mockProc;
    fs::path mockRootPath;

    void SetUp() override {
        const auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        fs::path basePath = fs::temp_directory_path() / "ProcAnalyzerTest" / 
                            (std::string(testInfo->test_case_name()) + "_" + std::string(testInfo->name()));
        
        if (fs::exists(basePath)) {
            fs::remove_all(basePath);
        }
        
        mockProc = std::make_unique<MockProc>(basePath.string());
        mockRootPath = mockProc->getPath();
    }
};

// --- Advanced Process Features Tests ---

TEST_F(MockProcTest, CreateExeSymlink) {
    const int testPid = 100;
    fs::path target = "/usr/bin/my_app";
    mockProc->createExeSymlink(testPid, target);
    fs::path symlinkPath = mockRootPath / std::to_string(testPid) / "exe";
    ASSERT_TRUE(fs::is_symlink(symlinkPath));
    ASSERT_EQ(fs::read_symlink(symlinkPath), target);
}

TEST_F(MockProcTest, CreateCwdSymlink) {
    const int testPid = 101;
    fs::path target = "/home/user/project";
    mockProc->createCwdSymlink(testPid, target);
    fs::path symlinkPath = mockRootPath / std::to_string(testPid) / "cwd";
    ASSERT_TRUE(fs::is_symlink(symlinkPath));
    ASSERT_EQ(fs::read_symlink(symlinkPath), target);
}

TEST_F(MockProcTest, CreateRootSymlink) {
    const int testPid = 102;
    fs::path target = "/";
    mockProc->createRootSymlink(testPid, target);
    fs::path symlinkPath = mockRootPath / std::to_string(testPid) / "root";
    ASSERT_TRUE(fs::is_symlink(symlinkPath));
    ASSERT_EQ(fs::read_symlink(symlinkPath), target);
}

TEST_F(MockProcTest, CreateComm) {
    const int testPid = 103;
    std::string commName = "my_daemon";
    mockProc->createComm(testPid, commName);
    fs::path commPath = mockRootPath / std::to_string(testPid) / "comm";
    ASSERT_TRUE(fs::exists(commPath));
    auto content = readFileContent(commPath);
    if (content) {
        EXPECT_EQ(content.value(), commName + "\n");
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, ProcMapEntryToString) {
    MockProc::ProcMapEntry entry = {
        .addressRange = "7f000000-7f010000",
        .perms = "r-xp",
        .offset = mapOffset,
        .dev = "08:01",
        .inode = inode,
        .pathname = "/usr/lib/mylib.so"
    };
    std::string expected = "7f000000-7f010000 r-xp 0000000000001000 08:01 12345 /usr/lib/mylib.so";
    ASSERT_EQ(entry.toString(), expected);
}

TEST_F(MockProcTest, CreateMaps) {
    const int testPid = 104;
    std::vector<MockProc::ProcMapEntry> entries = {
        {
            .addressRange = "7f000000-7f010000",
            .perms = "r-xp",
            .offset = mapOffset,
            .dev = "08:01",
            .inode = inode,
            .pathname = "/usr/lib/mylib.so"
        },
        {
            .addressRange = "7f010000-7f020000",
            .perms = "rw-p",
            .offset = 0x0,
            .dev = "00:00",
            .inode = 0,
            .pathname = "[anon_heap]"
        }
    };
    mockProc->createMaps(testPid, entries);
    fs::path mapsPath = mockRootPath / std::to_string(testPid) / "maps";
    ASSERT_TRUE(fs::exists(mapsPath));
    auto content = readFileContent(mapsPath);
    if (content) {
        std::string expectedContent = entries[0].toString() + "\n" + entries[1].toString() + "\n";
        EXPECT_EQ(content.value(), expectedContent);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, ProcIoStatsToString) {
    MockProc::ProcIoStats stats;
    stats.rchar = ioRchar;
    stats.wchar = ioWchar;
    stats.readBytes = ioReadBytes;
    stats.writeBytes = ioWriteBytes;
    std::string expected = 
        "rchar: 1000\n"
        "wchar: 2000\n"
        "syscr: 0\n"
        "syscw: 0\n"
        "read_bytes: 500\n"
        "write_bytes: 1500\n"
        "cancelled_write_bytes: 0\n";
    ASSERT_EQ(stats.toString(), expected);
}

TEST_F(MockProcTest, CreateIo) {
    const int testPid = 105;
    MockProc::ProcIoStats stats;
    stats.rchar = ioSmallRchar;
    stats.wchar = ioSmallWchar;
    mockProc->createIo(testPid, stats);
    fs::path ioPath = mockRootPath / std::to_string(testPid) / "io";
    ASSERT_TRUE(fs::exists(ioPath));
    auto content = readFileContent(ioPath);
    if (content) {
        EXPECT_EQ(content.value(), stats.toString());
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, ProcStatDataToString) {
    MockProc::ProcStatData data;
    data.pid = pid123;
    data.comm = "test_proc";
    data.state = 'S';
    data.ppid = 1;
    data.utime = utime100;
    data.stime = stime50;
    data.vsize = vsize;
    data.rss = rss1000;

    std::string statContent = data.toString();
    
    std::vector<std::string> parts;
    std::stringstream ss(statContent);
    std::string part;
    while (ss >> part) {
        parts.push_back(part);
    }
    
    ASSERT_EQ(parts.size(), 52); 
    ASSERT_EQ(std::stoi(parts[0]), data.pid);
    ASSERT_EQ(parts[1], "(" + data.comm + ")");
    ASSERT_EQ(parts[2][0], data.state);
    ASSERT_EQ(std::stoi(parts[3]), data.ppid);
    ASSERT_EQ(std::stoul(parts[13]), data.utime);
    ASSERT_EQ(std::stoul(parts[14]), data.stime);
    ASSERT_EQ(std::stoul(parts[22]), data.vsize);
    ASSERT_EQ(std::stol(parts[23]), data.rss);
}

TEST_F(MockProcTest, CreateStat) {
    const int testPid = 106;
    MockProc::ProcStatData data;
    data.pid = testPid;
    data.comm = "my_test";
    data.state = 'R';
    data.ppid = 1;
    data.utime = utime10;
    data.stime = stime20;

    mockProc->createStat(testPid, data);
    fs::path statPath = mockRootPath / std::to_string(testPid) / "stat";
    ASSERT_TRUE(fs::exists(statPath));
    auto content = readFileContent(statPath);
    if (content) {
        EXPECT_EQ(content.value(), data.toString());
    } else {
        FAIL() << "Expected to read file content.";
    }
}

// --- New addProcess tests ---

TEST_F(MockProcTest, AddProcessMinimal) {
    const int testPid = 200;
    MockProc::AddProcessOptions options;
    options.name = "min_proc";

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::is_directory(pidPath));
    ASSERT_TRUE(fs::exists(pidPath / "status"));
    auto statusContent = readFileContent(pidPath / "status");
    if (statusContent) {
        EXPECT_TRUE(statusContent.value().find("Name: min_proc") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
    
    ASSERT_TRUE(fs::exists(pidPath / "comm"));
    auto commContent = readFileContent(pidPath / "comm");
    if (commContent) {
        EXPECT_EQ(commContent.value(), "min_proc\n");
    } else {
        FAIL() << "Expected to read file content.";
    }

    // Cmdline should exist and contain the name as fallback
    ASSERT_TRUE(fs::exists(pidPath / "cmdline"));
    std::vector<std::string> expectedCmdline = {"min_proc"};
    ASSERT_EQ(readNullSeparatedStrings(pidPath / "cmdline"), expectedCmdline);
}

TEST_F(MockProcTest, AddProcessWithCmdline) {
    const int testPid = 201;
    MockProc::AddProcessOptions options;
    options.name = "cmd_proc";
    options.cmdlineArgs = {"/path/to/cmd_proc", "--config", "file.conf"};

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::is_directory(pidPath));
    ASSERT_TRUE(fs::exists(pidPath / "cmdline"));
    ASSERT_EQ(readNullSeparatedStrings(pidPath / "cmdline"), options.cmdlineArgs);
}

TEST_F(MockProcTest, AddProcessWithCmdlineFallbackToName) {
    const int testPid = 202;
    MockProc::AddProcessOptions options;
    options.name = "cmd_fallback";
    
    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::is_directory(pidPath));
    ASSERT_TRUE(fs::exists(pidPath / "cmdline"));
    ASSERT_EQ(readNullSeparatedStrings(pidPath / "cmdline"), std::vector<std::string>{"cmd_fallback"});
}

TEST_F(MockProcTest, AddProcessWithSymlinks) {
    const int testPid = 203;
    MockProc::AddProcessOptions options;
    options.name = "sym_proc";
    options.exePath = "/usr/bin/sym_app";
    options.cwdPath = "/tmp/working";
    options.rootPath = "/chroot/env";

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::is_directory(pidPath));
    ASSERT_TRUE(fs::is_symlink(pidPath / "exe"));
    ASSERT_EQ(fs::read_symlink(pidPath / "exe"), options.exePath);
    ASSERT_TRUE(fs::is_symlink(pidPath / "cwd"));
    ASSERT_EQ(fs::read_symlink(pidPath / "cwd"), options.cwdPath);
    ASSERT_TRUE(fs::is_symlink(pidPath / "root"));
    ASSERT_EQ(fs::read_symlink(pidPath / "root"), options.rootPath);
}

TEST_F(MockProcTest, AddProcessWithEnviron) {
    const int testPid = 204;
    MockProc::AddProcessOptions options;
    options.name = "env_proc";
    options.environVars = {{"LANG", "C"}, {"TERM", "xterm"}};

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::is_directory(pidPath));
    ASSERT_TRUE(fs::exists(pidPath / "environ"));
    auto environContent = readFileContent(pidPath / "environ");
    if (environContent) {
        EXPECT_TRUE(environContent.value().find("LANG=C") != std::string::npos);
        EXPECT_TRUE(environContent.value().find("TERM=xterm") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, AddProcessWithFdDir) {
    const int testPid = 205;
    MockProc::AddProcessOptions options;
    options.name = "fd_proc";
    options.fds = {{0, "/dev/null"}, {fd10, "/var/log/my.log"}};

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::is_directory(pidPath));
    ASSERT_TRUE(fs::is_directory(pidPath / "fd"));
    ASSERT_TRUE(fs::is_symlink(pidPath / "fd" / "0"));
    ASSERT_EQ(fs::read_symlink(pidPath / "fd" / "0"), "/dev/null");
    ASSERT_TRUE(fs::is_symlink(pidPath / "fd" / "10"));
    ASSERT_EQ(fs::read_symlink(pidPath / "fd" / "10"), "/var/log/my.log");
}

TEST_F(MockProcTest, AddProcessWithIoStats) {
    const int testPid = 206;
    MockProc::AddProcessOptions options;
    options.name = "io_proc";
    options.ioStats.rchar = ioRchar5000;
    options.ioStats.writeBytes = ioWriteBytes10000;

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::is_directory(pidPath));
    ASSERT_TRUE(fs::exists(pidPath / "io"));
    auto content = readFileContent(pidPath / "io");
    if (content) {
        EXPECT_TRUE(content.value().find("rchar: 5000") != std::string::npos);
        EXPECT_TRUE(content.value().find("write_bytes: 10000") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, AddProcessWithStatData) {
    const int testPid = 207;
    MockProc::AddProcessOptions options;
    options.name = "stat_proc";
    options.statData.pid = testPid;
    options.statData.comm = "stat_proc_comm";
    options.statData.state = 'S';
    options.statData.ppid = ppid10;
    options.statData.utime = utime123;
    options.statData.stime = stime45;

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::is_directory(pidPath));
    ASSERT_TRUE(fs::exists(pidPath / "stat"));
    auto statContent = readFileContent(pidPath / "stat");
    if (statContent) {
        EXPECT_TRUE(statContent.value().find(std::to_string(testPid) + " (stat_proc_comm) S 10 ") != std::string::npos);
        EXPECT_TRUE(statContent.value().find(" 123 45 ") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, AddProcessWithPopulateDefaultStat) {
    const int testPid = 208;
    MockProc::AddProcessOptions options;
    options.name = "default_stat_proc";
    options.populateDefaultStat = true;

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::is_directory(pidPath));
    ASSERT_TRUE(fs::exists(pidPath / "stat"));
    auto statContent = readFileContent(pidPath / "stat");
    if (statContent) {
        EXPECT_TRUE(statContent.value().find(std::to_string(testPid) + " (default_stat_proc) ") != std::string::npos);
        EXPECT_TRUE(statContent.value().find(" (default_stat_proc) R ") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, AddProcessCombinedOptions) {
    const int testPid = 209;
    MockProc::AddProcessOptions options;
    options.name = "combined_proc";
    options.cmdlineArgs = {"/usr/bin/combined", "-v"};
    options.exePath = "/usr/bin/combined_exe";
    options.cwdPath = "/home/test";
    options.environVars = {{"LANG", "C"}, {"TERM", "xterm"}};
    options.fds = {{1, "/dev/stdout"}};
    options.ioStats.rchar = 1;
    options.statData.pid = testPid;
    options.statData.comm = "combined_comm";
    options.statData.state = 'Z'; 
    options.populateDefaultStat = true;

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::exists(pidPath));

    ASSERT_EQ(readNullSeparatedStrings(pidPath / "cmdline"), options.cmdlineArgs);
    ASSERT_EQ(fs::read_symlink(pidPath / "exe"), options.exePath);
    ASSERT_EQ(fs::read_symlink(pidPath / "cwd"), options.cwdPath);
    
    auto environContent = readFileContent(pidPath / "environ");
    if (environContent) {
        EXPECT_TRUE(environContent.value().find("LANG=C") != std::string::npos);
        EXPECT_TRUE(environContent.value().find("TERM=xterm") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }

    ASSERT_TRUE(fs::is_symlink(pidPath / "fd" / "1"));
    ASSERT_EQ(fs::read_symlink(pidPath / "fd" / "1"), "/dev/stdout");
    
    auto ioContent = readFileContent(pidPath / "io");
    if (ioContent) {
        EXPECT_TRUE(ioContent.value().find("rchar: 1") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
    
    auto statContent = readFileContent(pidPath / "stat");
    if (statContent) {
        EXPECT_TRUE(statContent.value().find(std::to_string(testPid) + " (combined_comm) Z ") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }

    ASSERT_TRUE(fs::exists(pidPath / "comm"));
    auto commContent = readFileContent(pidPath / "comm");
    if (commContent) {
        EXPECT_EQ(commContent.value(), options.name + "\n");
    } else {
        FAIL() << "Expected to read file content.";
    }
}

// --- System Statistics and Builder API Tests ---

TEST_F(MockProcTest, ProcessBuilderFluentApi) {
    const int pid = 300;
    mockProc->buildProcess(pid)
        .withName("fluent_proc")
        .withParent(ppid50)
        .withCmdline({"/bin/fluent", "--mode=fast"})
        .create();

    fs::path pidPath = mockRootPath / std::to_string(pid);
    ASSERT_TRUE(fs::exists(pidPath));
    auto commContent = readFileContent(pidPath / "comm");
    if (commContent) {
        EXPECT_EQ(commContent.value(), "fluent_proc\n");
    } else {
        FAIL() << "Expected to read file content.";
    }
    
    auto statContent = readFileContent(pidPath / "stat");
    if (statContent) {
        EXPECT_TRUE(statContent.value().find(" 50 ") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
    
    std::vector<std::string> expectedCmd = {"/bin/fluent", "--mode=fast"};
    ASSERT_EQ(readNullSeparatedStrings(pidPath / "cmdline"), expectedCmd);
}

TEST_F(MockProcTest, ProcessBuilderWithMaps) {
    const int pid = 301;
    mockProc->buildProcess(pid)
        .withName("mapped_proc")
        .withMap({.addressRange="1000-2000", .perms="r-xp", .pathname="/lib/libc.so"})
        .create();
    
    fs::path mapsPath = mockRootPath / std::to_string(pid) / "maps";
    ASSERT_TRUE(fs::exists(mapsPath));
    auto mapsContent = readFileContent(mapsPath);
    if (mapsContent) {
        EXPECT_TRUE(mapsContent.value().find("1000-2000 r-xp") != std::string::npos);
        EXPECT_TRUE(mapsContent.value().find("/lib/libc.so") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, AddThread) {
    const int parentPid = 400;
    const int threadId = 401;
    
    mockProc->buildProcess(parentPid).withName("parent").create();
    
    MockProc::AddThreadOptions options;
    options.name = "child_thread";
    options.statData.pid = threadId;
    options.statData.state = 'S';
    
    mockProc->addThread(parentPid, threadId, options);
    
    fs::path threadPath = mockRootPath / std::to_string(threadId);
    fs::path taskPath = mockRootPath / std::to_string(parentPid) / "task" / std::to_string(threadId);
    
    ASSERT_TRUE(fs::exists(threadPath));
    ASSERT_TRUE(fs::exists(taskPath));
    
    auto statContent = readFileContent(threadPath / "stat");
    if (statContent) {
        EXPECT_TRUE(statContent.value().find("(child_thread)") != std::string::npos);
        EXPECT_TRUE(statContent.value().find(std::to_string(threadId)) != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
    
    ASSERT_TRUE(fs::is_symlink(threadPath / "exe"));
    fs::path symlinkTarget = fs::read_symlink(threadPath / "exe");
    EXPECT_EQ(symlinkTarget.string(), "../" + std::to_string(parentPid) + "/exe");
}
