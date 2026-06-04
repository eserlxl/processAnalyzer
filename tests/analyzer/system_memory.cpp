// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/system_model.h"
#include "utils/testing_framework.h" // For MockProc

#include <filesystem>
#include <memory>

class GetSystemMemoryInfoTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr unsigned long kMemTotal = 16384;
    static constexpr unsigned long kMemFree = 8192;
    static constexpr unsigned long kMemAvailable = 10000;
    static constexpr unsigned long kBuffers = 256;
    static constexpr unsigned long kCached = 4096;
    static constexpr unsigned long kSwapTotal = 2048;
    static constexpr unsigned long kSwapFree = 2000;

    GetSystemMemoryInfoTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_meminfo_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetSystemMemoryInfoTest, ParsesMeminfoFields) {
    MockProc::MeminfoData data;
    data.memTotalKb = kMemTotal;
    data.memFreeKb = kMemFree;
    data.memAvailableKb = kMemAvailable;
    data.buffersKb = kBuffers;
    data.cachedKb = kCached;
    data.swapTotalKb = kSwapTotal;
    data.swapFreeKb = kSwapFree;
    mockProc->createMeminfo(data);

    auto result = analyzer.getSystemMemoryInfo();
    ASSERT_TRUE(result.has_value());
    const SystemMemoryInfo& info = result.value();
    EXPECT_EQ(info.memTotal, kMemTotal);
    EXPECT_EQ(info.memFree, kMemFree);
    EXPECT_EQ(info.memAvailable, kMemAvailable);
    EXPECT_EQ(info.buffers, kBuffers);
    EXPECT_EQ(info.cached, kCached);
    EXPECT_EQ(info.swapTotal, kSwapTotal);
    EXPECT_EQ(info.swapFree, kSwapFree);
}

TEST_F(GetSystemMemoryInfoTest, MissingMeminfoReturnsError) {
    auto result = analyzer.getSystemMemoryInfo();
    EXPECT_FALSE(result.has_value());
}
