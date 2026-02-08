// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h" // Include Google Test framework
#include "TestUtils.h" // Include the header with MockProc
#include <vector>
#include <map>
#include <string>
#include <filesystem>
#include <fstream>
#include <sstream> // Required for stringstream
namespace fs = std::filesystem;

// Define constants for common magic numbers
constexpr unsigned long kbInBytes = 1024;
constexpr unsigned long mbInBytes = 1024 * kbInBytes;

// Constants for test values
constexpr unsigned long kMapOffset = 0x1000;
constexpr unsigned long kInode = 12345;
constexpr unsigned long kIoRchar = 1000;
constexpr unsigned long kIoWchar = 2000;
constexpr unsigned long kIoReadBytes = 500;
constexpr unsigned long kIoWriteBytes = 1500;
constexpr unsigned long kIoSmallRchar = 100;
constexpr unsigned long kIoSmallWchar = 200;
constexpr int kPid123 = 123;
constexpr unsigned long kUtime100 = 100;
constexpr unsigned long kStime50 = 50;
constexpr unsigned long kVsize = 1234567;
constexpr unsigned long kRss1000 = 1000;
constexpr unsigned long kUtime10 = 10;
constexpr unsigned long kStime20 = 20;
constexpr unsigned long kTotalMemGB = 16;
constexpr unsigned long kFreeMemGB = 8;
constexpr unsigned long kCachedMemGB = 2;
constexpr unsigned long kTotalMemMB = 1024;
constexpr unsigned long kFreeMemMB = 512;
constexpr float kCpuMhz = 2500.500F;
constexpr int kFd10 = 10;
constexpr unsigned long kIoRchar5000 = 5000;
constexpr unsigned long kIoWriteBytes10000 = 10000;
constexpr int kPpid10 = 10;
constexpr unsigned long kUtime123 = 123;
constexpr unsigned long kStime45 = 45;

// Iteration 12 constants
constexpr int kPpid50 = 50;
constexpr unsigned long long kUser100 = 100;
constexpr unsigned long long kIdle200 = 200;
constexpr unsigned long long kCtxt5000 = 5000;
constexpr unsigned long long kProcesses10 = 10;
constexpr unsigned long long kUser200 = 200;
constexpr unsigned long long kIdle400 = 400;
constexpr double kUptimeSec = 1234.56;
constexpr double kIdleSec = 789.01;
constexpr unsigned long long kRxBytes1000 = 1000;
constexpr unsigned long long kTxBytes2000 = 2000;

// Helper to read file content for verification
std::string readFileContent(const fs::path& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return ""; // Return empty string if file cannot be opened
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Helper to read null-terminated strings from a file
std::vector<std::string> readNullSeparatedStrings(const fs::path& filePath) {
    std::vector<std::string> result;
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return result;
    }

    std::string buffer;
    char c;
    while (file.get(c)) {
        if (c == '\0') {
            result.push_back(buffer);
            buffer.clear();
        } else {
            buffer += c;
        }
    }
    if (!buffer.empty()) { // Add last string if not null-terminated
        result.push_back(buffer);
    }
    return result;
}

// Test fixture for MockProc tests
class MockProcTest : public ::testing::Test {
protected:
    MockProc* mockProc;
    fs::path mockRootPath;

    void SetUp() override {
        // Removed random_seed as it's not directly available and can lead to build errors.
        // Using test case and test name should provide sufficient uniqueness.
        std::string basePath = std::string(::testing::UnitTest::GetInstance()->current_test_info()->test_case_name()) + "_" +
                               std::string(::testing::UnitTest::GetInstance()->current_test_info()->name());
        
        mockProc = new MockProc(basePath);
        mockRootPath = mockProc->getPath();
    }

    void TearDown() override {
        delete mockProc;
        // MockProc destructor already handles fs::remove_all(root)
    }
};

// --- Tests for existing Iteration 6 functionalities, now using GTest framework ---

TEST_F(MockProcTest, CreateFileAt) {
    fs::path relativeFilePath = "etc/config.conf";
    std::string fileContent = "key = value\n";
    mockProc->createFileAt(relativeFilePath, fileContent);
    fs::path expectedFilePath = mockRootPath / relativeFilePath;
    ASSERT_TRUE(fs::exists(expectedFilePath));
    ASSERT_EQ(readFileContent(expectedFilePath), fileContent);
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
        {"PPid", "123"}
    };
    mockProc->createStatus(testPid, statusData);
    fs::path statusPath = mockRootPath / std::to_string(testPid) / "status";
    ASSERT_TRUE(fs::exists(statusPath));
    std::string statusContent = readFileContent(statusPath);
    EXPECT_TRUE(statusContent.find("Name: my_process") != std::string::npos);
    EXPECT_TRUE(statusContent.find("State: R (running)") != std::string::npos);
    EXPECT_TRUE(statusContent.find("Pid: " + std::to_string(testPid)) != std::string::npos);
    EXPECT_TRUE(statusContent.find("PPid: 123") != std::string::npos);
}

TEST_F(MockProcTest, CreateEnviron) {
    const int testPid = 123;
    std::map<std::string, std::string> envVars = {
        {"PATH", "/usr/local/bin:/usr/bin:/bin"},
        {"USER", "testuser"},
        {"HOME", "/home/testuser"}
    };
    mockProc->createEnviron(testPid, envVars);
    fs::path environPath = mockRootPath / std::to_string(testPid) / "environ";
    ASSERT_TRUE(fs::exists(environPath));
    std::string environContent = readFileContent(environPath);
    EXPECT_TRUE(environContent.find("PATH=/usr/local/bin:/usr/bin:/bin") != std::string::npos);
    EXPECT_TRUE(environContent.find("USER=testuser") != std::string::npos);
    EXPECT_TRUE(environContent.find("HOME=/home/testuser") != std::string::npos);
    size_t nullCount = 0;
    for (char c : environContent) {
        if (c == '\0') nullCount++;
    }
    // The last variable does not have a null terminator in /proc/environ
    // If there are N variables, there are N-1 null terminators.
    ASSERT_EQ(nullCount, envVars.size() - 1);
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

// --- Iteration 8 New API Tests ---

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
    ASSERT_EQ(readFileContent(commPath), commName + "\n");
}

TEST_F(MockProcTest, ProcMapEntryToString) {
    MockProc::ProcMapEntry entry = {
        .addressRange = "7f000000-7f010000",
        .perms = "r-xp",
        .offset = kMapOffset,
        .dev = "08:01",
        .inode = kInode,
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
            .offset = kMapOffset,
            .dev = "08:01",
            .inode = kInode,
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
    std::string expectedContent = entries[0].toString() + "\n" + entries[1].toString() + "\n";
    ASSERT_EQ(readFileContent(mapsPath), expectedContent);
}

TEST_F(MockProcTest, ProcIoStatsToString) {
    MockProc::ProcIoStats stats;
    stats.rchar = kIoRchar;
    stats.wchar = kIoWchar;
    stats.readBytes = kIoReadBytes;
    stats.writeBytes = kIoWriteBytes;
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
    stats.rchar = kIoSmallRchar;
    stats.wchar = kIoSmallWchar;
    mockProc->createIo(testPid, stats);
    fs::path ioPath = mockRootPath / std::to_string(testPid) / "io";
    ASSERT_TRUE(fs::exists(ioPath));
    ASSERT_EQ(readFileContent(ioPath), stats.toString());
}

TEST_F(MockProcTest, ProcStatDataToString) {
    MockProc::ProcStatData data;
    data.pid = kPid123;
    data.comm = "test_proc";
    data.state = 'S';
    data.ppid = 1;
    // Set some non-default values to ensure they are formatted
    data.utime = kUtime100;
    data.stime = kStime50;
    data.vsize = kVsize;
    data.rss = kRss1000;

    // The toString() method must produce a string with all fields.
    // We'll check for key parts and general structure.
    std::string statContent = data.toString();
    EXPECT_TRUE(statContent.find("123 (test_proc) S 1 ") != std::string::npos); // pid, comm, state, ppid
    EXPECT_TRUE(statContent.find(" 100 50 ") != std::string::npos); // utime, stime
    EXPECT_TRUE(statContent.find(" 1234567 1000 ") != std::string::npos); // vsize, rss

    // More robust check: tokenize and compare
    std::vector<std::string> parts;
    std::stringstream ss(statContent);
    std::string part;
    while (ss >> part) {
        parts.push_back(part);
    }

    // A full /proc/stat has 52 fields (or more depending on kernel version)
    // We defined 48 fields in ProcStatData.
    ASSERT_GE(parts.size(), 48); 
    ASSERT_EQ(std::stoi(parts[0]), data.pid);
    ASSERT_EQ(parts[1], "(" + data.comm + ")");
    ASSERT_EQ(parts[2][0], data.state);
    ASSERT_EQ(std::stoi(parts[3]), data.ppid);
    // ... check other fields if precise comparison is needed
}

TEST_F(MockProcTest, CreateStat) {
    const int testPid = 106;
    MockProc::ProcStatData data;
    data.pid = testPid;
    data.comm = "my_test";
    data.state = 'R';
    data.ppid = 1;
    data.utime = kUtime10;
    data.stime = kStime20;

    mockProc->createStat(testPid, data);
    fs::path statPath = mockRootPath / std::to_string(testPid) / "stat";
    ASSERT_TRUE(fs::exists(statPath));
    ASSERT_EQ(readFileContent(statPath), data.toString());
}

TEST_F(MockProcTest, MeminfoDataToString) {
    MockProc::MeminfoData data;
    data.memTotalKb = static_cast<unsigned long>(kTotalMemGB) * mbInBytes; // 16GB in KB
    data.memFreeKb = static_cast<unsigned long>(kFreeMemGB) * mbInBytes;   // 8GB in KB
    data.cachedKb = static_cast<unsigned long>(kCachedMemGB) * mbInBytes;    // 2GB in KB
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
    data.memTotalKb = kTotalMemMB * kbInBytes; // 1024MB in KB
    data.memFreeKb = kFreeMemMB * kbInBytes;   // 512MB in KB
    mockProc->createMeminfo(data);
    fs::path meminfoPath = mockRootPath / "meminfo";
    ASSERT_TRUE(fs::exists(meminfoPath));
    ASSERT_EQ(readFileContent(meminfoPath), data.toString());
}

TEST_F(MockProcTest, CpuinfoDataToString) {
    MockProc::CpuinfoData core0;
    core0.processorId = 0;
    core0.modelName = "Intel Core i7";
    core0.cpuMhz = kCpuMhz;
    core0.cpuCores = 4;

    MockProc::CpuinfoData core1;
    core1.processorId = 1;
    core1.modelName = "Intel Core i7";
    core1.cpuMhz = kCpuMhz;
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
    core0.cpuMhz = kCpuMhz;
    cores.push_back(core0);

    MockProc::CpuinfoData core1;
    core1.processorId = 1;
    core1.modelName = "Intel Core i7";
    core1.cpuMhz = kCpuMhz;
    cores.push_back(core1);

    mockProc->createCpuinfo(cores);
    fs::path cpuinfoPath = mockRootPath / "cpuinfo";
    ASSERT_TRUE(fs::exists(cpuinfoPath));
    std::string expectedContent = core0.toString() + core1.toString();
    ASSERT_EQ(readFileContent(cpuinfoPath), expectedContent);
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
    EXPECT_TRUE(readFileContent(pidPath / "status").find("Name: min_proc") != std::string::npos);
    ASSERT_TRUE(fs::exists(pidPath / "comm"));
    EXPECT_EQ(readFileContent(pidPath / "comm"), "min_proc\n");
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
    std::string environContent = readFileContent(pidPath / "environ");
    EXPECT_TRUE(environContent.find("LANG=C") != std::string::npos);
    EXPECT_TRUE(environContent.find("TERM=xterm") != std::string::npos);
}

TEST_F(MockProcTest, AddProcessWithFdDir) {
    const int testPid = 205;
    MockProc::AddProcessOptions options;
    options.name = "fd_proc";
    options.fds = {{0, "/dev/null"}, {kFd10, "/var/log/my.log"}};

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
    options.ioStats.rchar = kIoRchar5000;
    options.ioStats.writeBytes = kIoWriteBytes10000;

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::is_directory(pidPath));
    ASSERT_TRUE(fs::exists(pidPath / "io"));
    EXPECT_TRUE(readFileContent(pidPath / "io").find("rchar: 5000") != std::string::npos);
    EXPECT_TRUE(readFileContent(pidPath / "io").find("write_bytes: 10000") != std::string::npos);
}

TEST_F(MockProcTest, AddProcessWithStatData) {
    const int testPid = 207;
    MockProc::AddProcessOptions options;
    options.name = "stat_proc";
    options.statData.pid = testPid;
    options.statData.comm = "stat_proc_comm";
    options.statData.state = 'S';
    options.statData.ppid = kPpid10;
    options.statData.utime = kUtime123;
    options.statData.stime = kStime45;

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::is_directory(pidPath));
    ASSERT_TRUE(fs::exists(pidPath / "stat"));
    std::string statContent = readFileContent(pidPath / "stat");
    EXPECT_TRUE(statContent.find(std::to_string(testPid) + " (stat_proc_comm) S 10 ") != std::string::npos);
    EXPECT_TRUE(statContent.find(" 123 45 ") != std::string::npos);
}

TEST_F(MockProcTest, AddProcessWithPopulateDefaultStat) {
    const int testPid = 208;
    MockProc::AddProcessOptions options;
    options.name = "default_stat_proc";
    options.populateDefaultStat = true;
    // Don't set statData explicitly, let it use defaults

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::is_directory(pidPath));
    ASSERT_TRUE(fs::exists(pidPath / "stat"));
    std::string statContent = readFileContent(pidPath / "stat");
    // Check that pid and comm are correctly defaulted
    EXPECT_TRUE(statContent.find(std::to_string(testPid) + " (default_stat_proc) ") != std::string::npos);
    // Check for default state 'R'
    EXPECT_TRUE(statContent.find(" (default_stat_proc) R ") != std::string::npos);
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
    options.statData.state = 'Z'; // Z for zombie process
    options.populateDefaultStat = true; // Should be overridden by explicit statData.pid/comm

    mockProc->addProcess(testPid, options);

    fs::path pidPath = mockRootPath / std::to_string(testPid);
    ASSERT_TRUE(fs::exists(pidPath));

    // Verify cmdline
    ASSERT_EQ(readNullSeparatedStrings(pidPath / "cmdline"), options.cmdlineArgs);
    // Verify exe symlink
    ASSERT_EQ(fs::read_symlink(pidPath / "exe"), options.exePath);
    // Verify cwd symlink
    ASSERT_EQ(fs::read_symlink(pidPath / "cwd"), options.cwdPath);
    // Verify environ
    std::string environContent = readFileContent(pidPath / "environ");
    EXPECT_TRUE(environContent.find("LANG=C") != std::string::npos);
    EXPECT_TRUE(environContent.find("TERM=xterm") != std::string::npos);
    // Verify fds
    ASSERT_TRUE(fs::is_symlink(pidPath / "fd" / "1"));
    ASSERT_EQ(fs::read_symlink(pidPath / "fd" / "1"), "/dev/stdout");
    // Verify io
    EXPECT_TRUE(readFileContent(pidPath / "io").find("rchar: 1") != std::string::npos);
    // Verify stat
    std::string statContent = readFileContent(pidPath / "stat");
    EXPECT_TRUE(statContent.find(std::to_string(testPid) + " (combined_comm) Z ") != std::string::npos);
    // Verify comm
    ASSERT_TRUE(fs::exists(pidPath / "comm"));
    EXPECT_EQ(readFileContent(pidPath / "comm"), options.name + "\n");
}

// --- Iteration 12 New API Tests ---

TEST_F(MockProcTest, ProcessBuilderFluentApi) {
    const int pid = 300;
    mockProc->buildProcess(pid)
        .withName("fluent_proc")
        .withParent(kPpid50)
        .withCmdline({"/bin/fluent", "--mode=fast"})
        .create();

    fs::path pidPath = mockRootPath / std::to_string(pid);
    ASSERT_TRUE(fs::exists(pidPath));
    // Check name (comm)
    EXPECT_EQ(readFileContent(pidPath / "comm"), "fluent_proc\n");
    // Check ppid in stat
    std::string statContent = readFileContent(pidPath / "stat");
    EXPECT_TRUE(statContent.find(" 50 ") != std::string::npos); // ppid 50
    // Check cmdline
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
    std::string mapsContent = readFileContent(mapsPath);
    EXPECT_TRUE(mapsContent.find("1000-2000 r-xp") != std::string::npos);
    EXPECT_TRUE(mapsContent.find("/lib/libc.so") != std::string::npos);
}

TEST_F(MockProcTest, AddThread) {
    const int parentPid = 400;
    const int threadId = 401;
    
    // First create parent
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
    
    // Check stat in thread dir
    std::string statContent = readFileContent(threadPath / "stat");
    EXPECT_TRUE(statContent.find("(child_thread)") != std::string::npos);
    EXPECT_TRUE(statContent.find(std::to_string(threadId)) != std::string::npos);
    
    // Check symlinks
    ASSERT_TRUE(fs::is_symlink(threadPath / "exe"));
    // Target should be related to parent (implementation specific, check logic in TestUtils.cpp)
    // In TestUtils.cpp, I implemented it as symlink to "../parentPid/exe"
    // However, create_symlink creates a symlink AT path pointing TO target.
    // If target is relative, it is relative to the directory containing the symlink.
    // threadPath is root/threadId. parentPath is root/parentPid.
    // ../parentPid/exe is correct relative path.
    
    // fs::read_symlink returns the target path stored in the link.
    // It does not resolve it to absolute path unless we ask.
    // So we expect "../400/exe"
    fs::path symlinkTarget = fs::read_symlink(threadPath / "exe");
    EXPECT_EQ(symlinkTarget.string(), "../" + std::to_string(parentPid) + "/exe");
}

TEST_F(MockProcTest, CreateSystemStat) {
    MockProc::SystemStatData data;
    data.user = kUser100;
    data.idle = kIdle200;
    data.ctxt = kCtxt5000;
    data.processes = kProcesses10;
    
    mockProc->createSystemStat(data);
    
    fs::path statPath = mockRootPath / "stat";
    ASSERT_TRUE(fs::exists(statPath));
    std::string content = readFileContent(statPath);
    EXPECT_TRUE(content.find("cpu  100 0 0 200") != std::string::npos);
    EXPECT_TRUE(content.find("ctxt 5000") != std::string::npos);
    EXPECT_TRUE(content.find("processes 10") != std::string::npos);
}

TEST_F(MockProcTest, CreatePerCpuStat) {
    std::vector<MockProc::SystemStatData> perCpu;
    MockProc::SystemStatData total;
    total.user = kUser200;
    total.idle = kIdle400;
    perCpu.push_back(total); // cpu
    
    MockProc::SystemStatData core0;
    core0.user = kUser100;
    core0.idle = kIdle200;
    perCpu.push_back(core0); // cpu0
    
    MockProc::SystemStatData core1;
    core1.user = kUser100;
    core1.idle = kIdle200;
    perCpu.push_back(core1); // cpu1
    
    mockProc->createSystemStat(perCpu);
    
    fs::path statPath = mockRootPath / "stat";
    std::string content = readFileContent(statPath);
    EXPECT_TRUE(content.find("cpu  200") != std::string::npos);
    EXPECT_TRUE(content.find("cpu0 100") != std::string::npos);
    EXPECT_TRUE(content.find("cpu1 100") != std::string::npos);
}

TEST_F(MockProcTest, CreateUptime) {
    mockProc->createUptime(kUptimeSec, kIdleSec);
    fs::path uptimePath = mockRootPath / "uptime";
    ASSERT_TRUE(fs::exists(uptimePath));
    std::string content = readFileContent(uptimePath);
    EXPECT_TRUE(content.find("1234.56 789.01") != std::string::npos);
}

TEST_F(MockProcTest, CreateVersion) {
    std::string ver = "Linux version 6.0.0-mock";
    mockProc->createVersion(ver);
    fs::path verPath = mockRootPath / "version";
    ASSERT_TRUE(fs::exists(verPath));
    EXPECT_EQ(readFileContent(verPath), ver + "\n");
}

TEST_F(MockProcTest, CreateNetDev) {
    std::vector<MockProc::NetDevStats> devs;
    MockProc::NetDevStats eth0;
    eth0.interface = "eth0";
    eth0.rx_bytes = kRxBytes1000;
    eth0.tx_bytes = kTxBytes2000;
    devs.push_back(eth0);
    
    mockProc->createNetDev(devs);
    
    fs::path netDevPath = mockRootPath / "net" / "dev";
    ASSERT_TRUE(fs::exists(netDevPath));
    std::string content = readFileContent(netDevPath);
    EXPECT_TRUE(content.find("eth0: 1000") != std::string::npos);
    EXPECT_TRUE(content.find("2000") != std::string::npos);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
