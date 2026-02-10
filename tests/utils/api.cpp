// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/Test.h"
#include <sstream>
#include <memory>
#include <optional>
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <ranges>

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
constexpr unsigned long totalMemGb = 16;
constexpr unsigned long freeMemGb = 8;
constexpr unsigned long cachedMemGb = 2;
constexpr unsigned long totalMemMb = 1024;
constexpr unsigned long freeMemMb = 512;
constexpr float cpuMhz = 2500.500F;
constexpr int fd10 = 10;
constexpr unsigned long ioRchar5000 = 5000;
constexpr unsigned long ioWriteBytes10000 = 10000;
constexpr int ppid10 = 10;
constexpr unsigned long utime123 = 123;
constexpr unsigned long stime45 = 45;

constexpr int ppid50 = 50;
constexpr unsigned long long user100 = 100;
constexpr unsigned long long idle200 = 200;
constexpr unsigned long long ctxt5000 = 5000;
constexpr unsigned long long processes10 = 10;
constexpr unsigned long long user200 = 200;
constexpr unsigned long long idle400 = 400;
constexpr double uptimeSec = 1234.56;
constexpr double idleSec = 789.01;
constexpr unsigned long long rxBytes1000 = 1000;
constexpr unsigned long long txBytes2000 = 2000;

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
    // This handles the case where the content does not end with a null
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
    return splitNullSeparatedStrings(*contentOpt);
}

// Helper to parse a key-value file (like /proc/status)
std::map<std::string, std::string> parseKeyValueFile(const fs::path& filePath) {
    std::map<std::string, std::string> data;
    auto contentOpt = readFileContent(filePath);
    if (!contentOpt) return data;

    std::stringstream ss(*contentOpt);
    std::string line;
    while (std::getline(ss, line)) {
        auto colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            // Trim leading whitespace from value
            auto firstChar = value.find_first_not_of(" \t");
            if (firstChar != std::string::npos) {
                value = value.substr(firstChar);
            }
            data[key] = value;
        }
    }
    return data;
}

} // namespace

// Test fixture for MockProc tests
class MockProcTest : public ::testing::Test {
protected:
    std::unique_ptr<MockProc> mockProc;
    fs::path mockRootPath;

    void SetUp() override {
        // Create a unique-enough temporary path for the test
        const auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        fs::path basePath = fs::temp_directory_path() / "ProcAnalyzerTest" / 
                            (std::string(testInfo->test_case_name()) + "_" + std::string(testInfo->name()));
        
        // Clean up any leftovers from previous runs
        if (fs::exists(basePath)) {
            fs::remove_all(basePath);
        }
        
        mockProc = std::make_unique<MockProc>(basePath.string());
        mockRootPath = mockProc->getPath();
    }
};

// --- Helper Function Tests ---
TEST(HelperTest, ReadFileContent_NonExistentFile) {
    auto content = readFileContent("a/file/that/does/not/exist.txt");
    ASSERT_FALSE(content.has_value());
}

TEST(HelperTest, SplitNullSeparatedStrings) {
    EXPECT_TRUE(splitNullSeparatedStrings("").empty());
    EXPECT_EQ(splitNullSeparatedStrings("abc"), std::vector<std::string>{"abc"});
    const std::array<char, 5> content1 = {'A', '\0', 'B', '\0', 'C'};
    EXPECT_EQ(splitNullSeparatedStrings(std::string(content1.data(), content1.size())), (std::vector<std::string>{"A", "B", "C"}));
    const std::array<char, 4> content2 = {'A', '\0', 'B', '\0'};
    EXPECT_EQ(splitNullSeparatedStrings(std::string(content2.data(), content2.size())), (std::vector<std::string>{"A", "B"}));
}


// --- Basic MockProc Functionality Tests ---

TEST_F(MockProcTest, CreateFileAt) {
    fs::path relativeFilePath = "etc/config.conf";
    std::string fileContent = "key = value\n";
    mockProc->createFileAt(relativeFilePath, fileContent);
    fs::path expectedFilePath = mockRootPath / relativeFilePath;
    ASSERT_TRUE(fs::exists(expectedFilePath));
    auto content = readFileContent(expectedFilePath);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(content.value(), fileContent);
}

TEST_F(MockProcTest, CreateFileAt_EmptyContent) {
    fs::path relativeFilePath = "etc/empty.conf";
    std::string fileContent;
    mockProc->createFileAt(relativeFilePath, fileContent);
    fs::path expectedFilePath = mockRootPath / relativeFilePath;
    ASSERT_TRUE(fs::exists(expectedFilePath));
    auto content = readFileContent(expectedFilePath);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(content.value(), fileContent);
}

TEST_F(MockProcTest, CreateDirectoryAt) {
    fs::path relativeDirPath = "data/logs";
    mockProc->createDirectoryAt(relativeDirPath);
    fs::path expectedDirPath = mockRootPath / relativeDirPath;
    ASSERT_TRUE(fs::is_directory(expectedDirPath));
}

TEST_F(MockProcTest, CreateSymlinkAt) {
    fs::path relativeSymlinkPath = "data/current_log";
    fs::path targetFilePath = "data/logs/app.log"; // Target relative to symlink's parent
    mockProc->createSymlinkAt(relativeSymlinkPath, targetFilePath);
    fs::path expectedSymlinkPath = mockRootPath / relativeSymlinkPath;
    ASSERT_TRUE(fs::is_symlink(expectedSymlinkPath));
    ASSERT_EQ(fs::read_symlink(expectedSymlinkPath).string(), targetFilePath.string());
}

TEST_F(MockProcTest, CreateCmdline) {
    const int testPid = 123;
    std::vector<std::string> cmdArgs = {"/usr/bin/my_process", "--verbose", "-f", "config.txt"};
    mockProc->createCmdline(testPid, cmdArgs);
    fs::path cmdlinePath = mockRootPath / std::to_string(testPid) / "cmdline";
    ASSERT_TRUE(fs::exists(cmdlinePath));
    ASSERT_EQ(readNullSeparatedStrings(cmdlinePath), cmdArgs);
}

TEST_F(MockProcTest, CreateStatus) {
    const int testPid = 123;
    std::map<std::string, std::string> statusData = {
        {"Name", "my_process"},
        {"State", "R (running)"},
        {"Pid", std::to_string(testPid)},
        {"PPid", "1"}
    };
    mockProc->createStatus(testPid, statusData);
    fs::path statusPath = mockRootPath / std::to_string(testPid) / "status";
    ASSERT_TRUE(fs::exists(statusPath));

    auto parsedData = parseKeyValueFile(statusPath);
    ASSERT_EQ(parsedData.size(), statusData.size());
    for(const auto& pair : statusData) {
        ASSERT_TRUE(parsedData.count(pair.first));
        EXPECT_EQ(parsedData[pair.first], pair.second);
    }
}

TEST_F(MockProcTest, CreateEnviron) {
    const int testPid = 123;
    std::map<std::string, std::string> envVarsMap = {
        {"PATH", "/usr/local/bin:/usr/bin:/bin"},
        {"USER", "testuser"},
        {"HOME", "/home/testuser"}
    };
    mockProc->createEnviron(testPid, envVarsMap);
    fs::path environPath = mockRootPath / std::to_string(testPid) / "environ";
    ASSERT_TRUE(fs::exists(environPath));

    auto actualVars = readNullSeparatedStrings(environPath);
    std::vector<std::string> expectedVars;
    expectedVars.reserve(envVarsMap.size());
    for(const auto& pair : envVarsMap) {
        expectedVars.push_back(pair.first + "=" + pair.second);
    }

    std::ranges::sort(actualVars);
    std::ranges::sort(expectedVars);

    ASSERT_EQ(actualVars, expectedVars);
}

TEST_F(MockProcTest, CreateFdDir) {
    const int testPid = 123;
    const int fdNum = 5;
    std::vector<std::pair<int, std::string>> fds = {
        {0, "/dev/stdin"},
        {1, "/dev/stdout"},
        {2, "/dev/stderr"},
        {fdNum, "/path/to/some/file"}
    };
    mockProc->createFdDir(testPid, fds);
    fs::path fdDirPath = mockRootPath / std::to_string(testPid) / "fd";
    ASSERT_TRUE(fs::is_directory(fdDirPath));
    for (const auto& fdPair : fds) {
        fs::path symlinkPath = fdDirPath / std::to_string(fdPair.first);
        ASSERT_TRUE(fs::is_symlink(symlinkPath));
        ASSERT_EQ(fs::read_symlink(symlinkPath).string(), fdPair.second);
    }
}

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
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(content.value(), commName + "\n");
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
    std::string expected = "7f000000-7f010000 r-xp 00001000 08:01 12345 /usr/lib/mylib.so";
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
    ASSERT_TRUE(content.has_value());
    std::string expectedContent = entries[0].toString() + "\n" + entries[1].toString() + "\n";
    EXPECT_EQ(*content, expectedContent);
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
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, stats.toString());
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
    
    // A full /proc/[pid]/stat has 52 fields.
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
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, data.toString());
}

TEST_F(MockProcTest, MeminfoDataToString) {
    MockProc::MeminfoData data;
    data.memTotalKb = static_cast<unsigned long>(totalMemGb) * mbInBytes; // 16GB in KB
    data.memFreeKb = static_cast<unsigned long>(freeMemGb) * mbInBytes;   // 8GB in KB
    data.cachedKb = static_cast<unsigned long>(cachedMemGb) * mbInBytes;    // 2GB in KB
    std::string expected = 
        "MemTotal:       16777216 kB\n"
        "MemFree:        8388608 kB\n"
        "MemAvailable:   0 kB\n" // Default value
        "Buffers:        0 kB\n" // Default value
        "Cached:         2097152 kB\n"
        "SwapTotal:      0 kB\n" // Default value
        "SwapFree:       0 kB\n"; // Default value
    ASSERT_EQ(data.toString(), expected);
}

TEST_F(MockProcTest, CreateMeminfo) {
    MockProc::MeminfoData data;
    data.memTotalKb = totalMemMb * kbInBytes; // 1024MB in KB
    data.memFreeKb = freeMemMb * kbInBytes;   // 512MB in KB
    mockProc->createMeminfo(data);
    fs::path meminfoPath = mockRootPath / "meminfo";
    ASSERT_TRUE(fs::exists(meminfoPath));
    auto content = readFileContent(meminfoPath);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, data.toString());
}

TEST_F(MockProcTest, CpuinfoDataToString) {
    MockProc::CpuinfoData core0;
    core0.processorId = 0;
    core0.modelName = "Intel Core i7";
    core0.cpuMhz = cpuMhz;
    core0.cpuCores = 4;

    MockProc::CpuinfoData core1;
    core1.processorId = 1;
    core1.modelName = "Intel Core i7";
    core1.cpuMhz = cpuMhz;
    core1.cpuCores = 4;

    std::string expected0 = 
        "processor\t: 0\n"
        "vendor_id\t: GenuineIntel\n"
        "model name\t: Intel Core i7\n"
        "cpu MHz\t\t: 2500.500\n"
        "siblings\t: 12\n" // Default
        "cpu cores\t: 4\n\n";

    ASSERT_EQ(core0.toString(), expected0);
}

TEST_F(MockProcTest, CreateCpuinfo) {
    std::vector<MockProc::CpuinfoData> cores;
    MockProc::CpuinfoData core0;
    core0.processorId = 0;
    core0.modelName = "Intel Core i7";
    core0.cpuMhz = cpuMhz;
    cores.push_back(core0);

    MockProc::CpuinfoData core1;
    core1.processorId = 1;
    core1.modelName = "Intel Core i7";
    core1.cpuMhz = cpuMhz;
    cores.push_back(core1);

    mockProc->createCpuinfo(cores);
    fs::path cpuinfoPath = mockRootPath / "cpuinfo";
    ASSERT_TRUE(fs::exists(cpuinfoPath));
    auto content = readFileContent(cpuinfoPath);
    ASSERT_TRUE(content.has_value());
    std::string expectedContent = core0.toString() + core1.toString();
    EXPECT_EQ(*content, expectedContent);
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
    ASSERT_TRUE(statusContent.has_value());
    EXPECT_TRUE(statusContent->find("Name: min_proc") != std::string::npos);
    
    ASSERT_TRUE(fs::exists(pidPath / "comm"));
    auto commContent = readFileContent(pidPath / "comm");
    ASSERT_TRUE(commContent.has_value());
    EXPECT_EQ(*commContent, "min_proc\n");

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
    // cmdlineArgs is empty, but name is provided, so cmdline should be created with just name
    
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
    ASSERT_TRUE(environContent.has_value());
    EXPECT_TRUE(environContent->find("LANG=C") != std::string::npos);
    EXPECT_TRUE(environContent->find("TERM=xterm") != std::string::npos);
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
    ASSERT_TRUE(content.has_value());
    EXPECT_TRUE(content->find("rchar: 5000") != std::string::npos);
    EXPECT_TRUE(content->find("write_bytes: 10000") != std::string::npos);
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
    ASSERT_TRUE(statContent.has_value());
    EXPECT_TRUE(statContent->find(std::to_string(testPid) + " (stat_proc_comm) S 10 ") != std::string::npos);
    EXPECT_TRUE(statContent->find(" 123 45 ") != std::string::npos);
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
    ASSERT_TRUE(statContent.has_value());
    EXPECT_TRUE(statContent->find(std::to_string(testPid) + " (default_stat_proc) ") != std::string::npos);
    EXPECT_TRUE(statContent->find(" (default_stat_proc) R ") != std::string::npos);
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
    ASSERT_TRUE(environContent.has_value());
    EXPECT_TRUE(environContent->find("LANG=C") != std::string::npos);
    EXPECT_TRUE(environContent->find("TERM=xterm") != std::string::npos);

    ASSERT_TRUE(fs::is_symlink(pidPath / "fd" / "1"));
    ASSERT_EQ(fs::read_symlink(pidPath / "fd" / "1"), "/dev/stdout");
    
    auto ioContent = readFileContent(pidPath / "io");
    ASSERT_TRUE(ioContent.has_value());
    EXPECT_TRUE(ioContent->find("rchar: 1") != std::string::npos);
    
    auto statContent = readFileContent(pidPath / "stat");
    ASSERT_TRUE(statContent.has_value());
    EXPECT_TRUE(statContent->find(std::to_string(testPid) + " (combined_comm) Z ") != std::string::npos);

    ASSERT_TRUE(fs::exists(pidPath / "comm"));
    auto commContent = readFileContent(pidPath / "comm");
    ASSERT_TRUE(commContent.has_value());
    EXPECT_EQ(*commContent, options.name + "\n");
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
    ASSERT_TRUE(commContent.has_value());
    EXPECT_EQ(*commContent, "fluent_proc\n");
    
    auto statContent = readFileContent(pidPath / "stat");
    ASSERT_TRUE(statContent.has_value());
    EXPECT_TRUE(statContent->find(" 50 ") != std::string::npos);
    
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
    ASSERT_TRUE(mapsContent.has_value());
    EXPECT_TRUE(mapsContent->find("1000-2000 r-xp") != std::string::npos);
    EXPECT_TRUE(mapsContent->find("/lib/libc.so") != std::string::npos);
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
    ASSERT_TRUE(statContent.has_value());
    EXPECT_TRUE(statContent->find("(child_thread)") != std::string::npos);
    EXPECT_TRUE(statContent->find(std::to_string(threadId)) != std::string::npos);
    
    ASSERT_TRUE(fs::is_symlink(threadPath / "exe"));
    fs::path symlinkTarget = fs::read_symlink(threadPath / "exe");
    EXPECT_EQ(symlinkTarget.string(), "../" + std::to_string(parentPid) + "/exe");
}

TEST_F(MockProcTest, CreateSystemStat) {
    MockProc::SystemStatData data;
    data.user = user100;
    data.idle = idle200;
    data.ctxt = ctxt5000;
    data.processes = processes10;
    
    mockProc->createSystemStat(data);
    
    fs::path statPath = mockRootPath / "stat";
    ASSERT_TRUE(fs::exists(statPath));
    auto content = readFileContent(statPath);
    ASSERT_TRUE(content.has_value());
    EXPECT_TRUE(content->find("cpu  100 0 0 200") != std::string::npos);
    EXPECT_TRUE(content->find("ctxt 5000") != std::string::npos);
    EXPECT_TRUE(content->find("processes 10") != std::string::npos);
}

TEST_F(MockProcTest, CreatePerCpuStat) {
    std::vector<MockProc::SystemStatData> perCpu;
    perCpu.reserve(3);
    MockProc::SystemStatData total;
    total.user = user200;
    total.idle = idle400;
    perCpu.push_back(total);
    
    MockProc::SystemStatData core0;
    core0.user = user100;
    core0.idle = idle200;
    perCpu.push_back(core0);
    
    MockProc::SystemStatData core1;
    core1.user = user100;
    core1.idle = idle200;
    perCpu.push_back(core1);
    
    mockProc->createSystemStat(perCpu);
    
    fs::path statPath = mockRootPath / "stat";
    auto content = readFileContent(statPath);
    ASSERT_TRUE(content.has_value());
    EXPECT_TRUE(content->find("cpu  200") != std::string::npos);
    EXPECT_TRUE(content->find("cpu0 100") != std::string::npos);
    EXPECT_TRUE(content->find("cpu1 100") != std::string::npos);
}

TEST_F(MockProcTest, CreateUptime) {
    mockProc->createUptime(uptimeSec, idleSec);
    fs::path uptimePath = mockRootPath / "uptime";
    ASSERT_TRUE(fs::exists(uptimePath));
    auto content = readFileContent(uptimePath);
    ASSERT_TRUE(content.has_value());
    EXPECT_TRUE(content->find("1234.56 789.01") != std::string::npos);
}

TEST_F(MockProcTest, CreateVersion) {
    std::string ver = "Linux version 6.0.0-mock";
    mockProc->createVersion(ver);
    fs::path verPath = mockRootPath / "version";
    ASSERT_TRUE(fs::exists(verPath));
    auto content = readFileContent(verPath);
    ASSERT_TRUE(content.has_value());
    EXPECT_EQ(*content, ver + "\n");
}

TEST_F(MockProcTest, CreateNetDev) {
    std::vector<MockProc::NetDevStats> devs;
    devs.reserve(1);
    MockProc::NetDevStats eth0;
    eth0.interface = "eth0";
    eth0.rx_bytes = rxBytes1000;
    eth0.tx_bytes = txBytes2000;
    devs.push_back(eth0);
    
    mockProc->createNetDev(devs);
    
    fs::path netDevPath = mockRootPath / "net" / "dev";
    ASSERT_TRUE(fs::exists(netDevPath));
    auto content = readFileContent(netDevPath);
    ASSERT_TRUE(content.has_value());
    EXPECT_TRUE(content->find("eth0: 1000") != std::string::npos);
    EXPECT_TRUE(content->find("2000") != std::string::npos);
}
