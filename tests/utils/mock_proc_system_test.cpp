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
constexpr unsigned long utime10 = 10;
constexpr unsigned long stime20 = 20;
constexpr unsigned long totalMemMb = 1024;
constexpr unsigned long freeMemMb = 512;
constexpr float cpuMhz = 2500.500F;

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

TEST_F(MockProcTest, MeminfoDataToString) {
    MockProc::MeminfoData data;
    data.memTotalKb = 16 * mbInBytes; // 16GB in KB
    data.memFreeKb = 8 * mbInBytes;   // 8GB in KB
    data.cachedKb = 2 * mbInBytes;    // 2GB in KB
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
