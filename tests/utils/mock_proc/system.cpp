// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/testing_framework.h" // Assuming MockProc is defined here
#include <sstream>
#include <memory>
#include <optional>
#include <vector>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

namespace {

// Define constants for common magic numbers
[[maybe_unused]] constexpr unsigned long kbInBytes = 1024;
[[maybe_unused]] constexpr unsigned long mbInBytes = 1024 * kbInBytes;
[[maybe_unused]] constexpr int sixteenGB = 16;
[[maybe_unused]] constexpr int eightGB = 8;
[[maybe_unused]] constexpr int twoGB = 2;


// Constants for test values
[[maybe_unused]] constexpr unsigned long totalMemMb = 1024;
[[maybe_unused]] constexpr unsigned long freeMemMb = 512;
[[maybe_unused]] constexpr float cpuMhz = 2500.500F;

[[maybe_unused]] constexpr unsigned long long user100 = 100;
[[maybe_unused]] constexpr unsigned long long idle200 = 200;
[[maybe_unused]] constexpr unsigned long long ctxt5000 = 5000;
[[maybe_unused]] constexpr unsigned long long processes10 = 10;
[[maybe_unused]] constexpr unsigned long long user200 = 200;
[[maybe_unused]] constexpr unsigned long long idle400 = 400;
[[maybe_unused]] constexpr double uptimeSec = 1234.56;
[[maybe_unused]] constexpr double idleSec = 789.01;
[[maybe_unused]] constexpr unsigned long long rxBytes1000 = 1000;
[[maybe_unused]] constexpr unsigned long long txBytes2000 = 2000;

// Constants for magic numbers in tests
[[maybe_unused]] constexpr unsigned long long memTotalDefault = 10000;
[[maybe_unused]] constexpr unsigned long long userDefault = 999;
[[maybe_unused]] constexpr unsigned long long rxBytesDefault = 100;
[[maybe_unused]] constexpr unsigned long long memTotalUnixTest = 1024;
[[maybe_unused]] constexpr unsigned long long memTotalInterleaved = 1000;
[[maybe_unused]] constexpr unsigned long long memFreeInterleaved = 500;
[[maybe_unused]] constexpr double uptimeInterleaved = 100.0;
[[maybe_unused]] constexpr double idleTimeInterleaved = 50.0;

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

    void TearDown() override {
        if (fs::exists(mockRootPath)) {
            fs::remove_all(mockRootPath);
        }
    }
};

TEST_F(MockProcTest, MeminfoDataToString) {
    MockProc::MeminfoData data;
    data.memTotalKb = sixteenGB * mbInBytes; // 16GB in KB
    data.memFreeKb = eightGB * mbInBytes;   // 8GB in KB
    data.cachedKb = twoGB * mbInBytes;    // 2GB in KB
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
    if (content) {
        EXPECT_EQ(*content, data.toString());
    } else {
        FAIL() << "Expected to read file content.";
    }
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
        "siblings\t: 12\n" // Default value from CpuinfoData struct
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
    if (content) {
        std::string expectedContent = core0.toString() + core1.toString();
        EXPECT_EQ(*content, expectedContent);
    } else {
        FAIL() << "Expected to read file content.";
    }
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
    if (content) {
        EXPECT_TRUE(content->find("cpu  100 0 0 200") != std::string::npos);
        EXPECT_TRUE(content->find("ctxt 5000") != std::string::npos);
        EXPECT_TRUE(content->find("processes 10") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
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
    if (content) {
        EXPECT_TRUE(content->find("cpu  200") != std::string::npos);
        EXPECT_TRUE(content->find("cpu0 100") != std::string::npos);
        EXPECT_TRUE(content->find("cpu1 100") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, CreateUptime) {
    mockProc->createUptime(uptimeSec, idleSec);
    fs::path uptimePath = mockRootPath / "uptime";
    ASSERT_TRUE(fs::exists(uptimePath));
    auto content = readFileContent(uptimePath);
    if (content) {
        EXPECT_TRUE(content->find("1234.56 789.01") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, CreateVersion) {
    std::string ver = "Linux version 6.0.0-mock";
    mockProc->createVersion(ver);
    fs::path verPath = mockRootPath / "version";
    ASSERT_TRUE(fs::exists(verPath));
    auto content = readFileContent(verPath);
    if (content) {
        EXPECT_EQ(*content, ver + "\n");
    } else {
        FAIL() << "Expected to read file content.";
    }
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
    if (content) {
        EXPECT_TRUE(content->find("eth0:") != std::string::npos);
        EXPECT_TRUE(content->find("1000") != std::string::npos);
        EXPECT_TRUE(content->find("2000") != std::string::npos);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

// Empty input scenarios
TEST_F(MockProcTest, CreateCpuinfoEmpty) {
    std::vector<MockProc::CpuinfoData> emptyCores;
    mockProc->createCpuinfo(emptyCores);
    fs::path cpuinfoPath = mockRootPath / "cpuinfo";
    ASSERT_TRUE(fs::exists(cpuinfoPath));
    auto content = readFileContent(cpuinfoPath);
    if (content) {
        EXPECT_EQ(*content, ""); // Expect an empty file
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, CreateNetDevEmpty) {
    std::vector<MockProc::NetDevStats> emptyDevs;
    mockProc->createNetDev(emptyDevs);
    fs::path netDevPath = mockRootPath / "net" / "dev";
    ASSERT_TRUE(fs::exists(netDevPath));
    auto content = readFileContent(netDevPath);
    if (content) {
        // Expect only the header of the net/dev file
        std::string expectedHeader = 
            "Inter-|   Receive                                                |  Transmit\n"
            " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n";
        EXPECT_EQ(*content, expectedHeader);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, CreateSystemStatEmptyPerCpu) {
    std::vector<MockProc::SystemStatData> emptyPerCpu;
    mockProc->createSystemStat(emptyPerCpu); 
    fs::path statPath = mockRootPath / "stat";
    ASSERT_TRUE(fs::exists(statPath));
    auto content = readFileContent(statPath);
    if (content) {
        std::string expectedContent =
            "cpu  0 0 0 0 0 0 0 0 0 0\n"
            "processes 0\n"
            "ctxt 0\n";
        EXPECT_EQ(*content, expectedContent);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

// Edge cases for data values
TEST_F(MockProcTest, MeminfoDataZeroValues) {
    MockProc::MeminfoData data;
    // All fields will be 0 by default. Explicitly set to 0 for clarity.
    data.memTotalKb = 0;
    data.memFreeKb = 0;
    data.memAvailableKb = 0;
    data.buffersKb = 0;
    data.cachedKb = 0;
    data.swapTotalKb = 0;
    data.swapFreeKb = 0;

    mockProc->createMeminfo(data);
    fs::path meminfoPath = mockRootPath / "meminfo";
    ASSERT_TRUE(fs::exists(meminfoPath));
    auto content = readFileContent(meminfoPath);
    if (content) {
        std::string expected = 
            "MemTotal:       0 kB\n"
            "MemFree:        0 kB\n"
            "MemAvailable:   0 kB\n"
            "Buffers:        0 kB\n"
            "Cached:         0 kB\n"
            "SwapTotal:      0 kB\n"
            "SwapFree:       0 kB\n";
        EXPECT_EQ(*content, expected);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, SystemStatDataZeroValues) {
    MockProc::SystemStatData data; // All members default to 0
    mockProc->createSystemStat(data);
    fs::path statPath = mockRootPath / "stat";
    ASSERT_TRUE(fs::exists(statPath));
    auto content = readFileContent(statPath);
    if (content) {
        std::string expectedContent = 
            "cpu  0 0 0 0 0 0 0 0 0 0\n"
            "processes 0\n"
            "ctxt 0\n";
        EXPECT_EQ(*content, expectedContent);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, NetDevStatsZeroValues) {
    std::vector<MockProc::NetDevStats> devs;
    MockProc::NetDevStats eth0;
    eth0.interface = "eth0";
    // All other members default to 0
    devs.push_back(eth0);
    
    mockProc->createNetDev(devs);
    
    fs::path netDevPath = mockRootPath / "net" / "dev";
    ASSERT_TRUE(fs::exists(netDevPath));
    auto content = readFileContent(netDevPath);
    if (content) {
        std::string expected = 
            "Inter-|   Receive                                                |  Transmit\n"
            " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
            "   eth0:        0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0\n";
        EXPECT_EQ(*content, expected);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

// Default values tests
TEST_F(MockProcTest, MeminfoDataDefaultValues) {
    MockProc::MeminfoData data;
    data.memTotalKb = memTotalDefault; // Set only one field
    // Other fields should take their default values (0)

    mockProc->createMeminfo(data);
    fs::path meminfoPath = mockRootPath / "meminfo";
    ASSERT_TRUE(fs::exists(meminfoPath));
    auto content = readFileContent(meminfoPath);
    if (content) {
        std::string expected = 
            "MemTotal:       " + std::to_string(memTotalDefault) + " kB\n"
            "MemFree:        0 kB\n"
            "MemAvailable:   0 kB\n"
            "Buffers:        0 kB\n"
            "Cached:         0 kB\n"
            "SwapTotal:      0 kB\n"
            "SwapFree:       0 kB\n";
        EXPECT_EQ(*content, expected);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, CpuinfoDataExplicitDefaults) {
    MockProc::CpuinfoData core0;
    core0.processorId = 0;
    core0.modelName = "Test CPU"; 
    // Other fields use their defaults: vendorId, cpuMhz, siblings, cpuCores

    mockProc->createCpuinfo({core0});
    fs::path cpuinfoPath = mockRootPath / "cpuinfo";
    ASSERT_TRUE(fs::exists(cpuinfoPath));
    auto content = readFileContent(cpuinfoPath);
    if (content) {
        // These defaults come from include/utils/test.h
        // std::string vendorId = "GenuineIntel";
        // float cpuMhz = 2600.000;
        // int siblings = 12;
        // int cpuCores = 6;
        std::string expected = 
            "processor\t: 0\n"
            "vendor_id\t: GenuineIntel\n"
            "model name\t: Test CPU\n"
            "cpu MHz\t\t: 2600.000\n"
            "siblings\t: 12\n" 
            "cpu cores\t: 6\n\n";
        EXPECT_EQ(*content, expected);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, SystemStatDataDefaultValues) {
    MockProc::SystemStatData data;
    data.user = userDefault; // Set only one field
    // Other fields should take their default values (0)

    mockProc->createSystemStat(data);
    fs::path statPath = mockRootPath / "stat";
    ASSERT_TRUE(fs::exists(statPath));
    auto content = readFileContent(statPath);
    if (content) {
        std::string expectedContent = 
            "cpu  " + std::to_string(userDefault) + " 0 0 0 0 0 0 0 0 0\n" // user set, others default 0
            "processes 0\n"
            "ctxt 0\n";
        EXPECT_EQ(*content, expectedContent);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

TEST_F(MockProcTest, NetDevStatsDefaultValues) {
    std::vector<MockProc::NetDevStats> devs;
    MockProc::NetDevStats eth0;
    eth0.interface = "default_eth0";
    eth0.rx_bytes = rxBytesDefault; // Set only two fields
    // All other rx/tx fields default to 0
    devs.push_back(eth0);
    
    mockProc->createNetDev(devs);
    
    fs::path netDevPath = mockRootPath / "net" / "dev";
    ASSERT_TRUE(fs::exists(netDevPath));
    auto content = readFileContent(netDevPath);
    if (content) {
        std::string expected = 
            "Inter-|   Receive                                                |  Transmit\n"
            " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
            "   default_eth0:    100       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0\n";
        EXPECT_EQ(*content, expected);
    } else {
        FAIL() << "Expected to read file content.";
    }
}

// Line ending consistency
TEST_F(MockProcTest, UnixLineEndings) {
    // Create a simple meminfo file
    MockProc::MeminfoData data;
    data.memTotalKb = memTotalUnixTest;
    mockProc->createMeminfo(data);

    fs::path meminfoPath = mockRootPath / "meminfo";
    ASSERT_TRUE(fs::exists(meminfoPath));
    
    std::ifstream file(meminfoPath, std::ios::in | std::ios::binary);
    std::string line;
    std::getline(file, line); // Read the first line

    // Check that there are no carriage returns on the first line
    EXPECT_EQ(line.find('\r'), std::string::npos) << "File should not contain carriage returns.";

    // Read the entire file content as a string
    auto content = readFileContent(meminfoPath);
    ASSERT_TRUE(content.has_value());
    
    // Explicitly check for newline characters
    // Example: "MemTotal:       1024 kB\nMemFree:        0 kB\n..."
    // Each logical line should end with '\n'
    if (content.has_value()) {
        EXPECT_TRUE(content->find('\n') != std::string::npos); // Ensure at least one newline
    }
}

// Interleaving mock creations
TEST_F(MockProcTest, InterleavedMockCreations) {
    // 1. Create Meminfo
    MockProc::MeminfoData memData;
    memData.memTotalKb = memTotalInterleaved;
    memData.memFreeKb = memFreeInterleaved;
    mockProc->createMeminfo(memData);

    // 2. Create Cpuinfo
    std::vector<MockProc::CpuinfoData> cores;
    MockProc::CpuinfoData core0;
    core0.processorId = 0;
    core0.modelName = "Interleaved CPU";
    cores.push_back(core0);
    mockProc->createCpuinfo(cores);

    // 3. Create Uptime
    mockProc->createUptime(uptimeInterleaved, idleTimeInterleaved);

    // Verify all files exist and contain correct content
    // Verify meminfo
    fs::path meminfoPath = mockRootPath / "meminfo";
    ASSERT_TRUE(fs::exists(meminfoPath));
    auto memContent = readFileContent(meminfoPath);
    ASSERT_TRUE(memContent.has_value());
    if (memContent.has_value()) {
        EXPECT_EQ(*memContent, memData.toString());
    }

    // Verify cpuinfo
    fs::path cpuinfoPath = mockRootPath / "cpuinfo";
    ASSERT_TRUE(fs::exists(cpuinfoPath));
    auto cpuContent = readFileContent(cpuinfoPath);
    ASSERT_TRUE(cpuContent.has_value());
    if (cpuContent.has_value()) {
        EXPECT_EQ(*cpuContent, core0.toString());
    }

    // Verify uptime
    fs::path uptimePath = mockRootPath / "uptime";
    ASSERT_TRUE(fs::exists(uptimePath));
    auto uptimeContent = readFileContent(uptimePath);
    ASSERT_TRUE(uptimeContent.has_value());
    
    if (uptimeContent.has_value()) {
        std::stringstream uptimeStream;
        uptimeStream << std::fixed << std::setprecision(2) << uptimeInterleaved << " " << idleTimeInterleaved << "\n";
        EXPECT_EQ(*uptimeContent, uptimeStream.str());
    }
}
