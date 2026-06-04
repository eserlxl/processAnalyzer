// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/system_model.h"
#include "utils/testing_framework.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

class GetSystemDiskIoStatsTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    GetSystemDiskIoStatsTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_diskio_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetSystemDiskIoStatsTest, ParsesDiskstatsEntry) {
    mockProc->createFileAt("diskstats",
        "   8   0 sda 1000 200 50000 1200 500 100 20000 600 0 800 1800 0 0 0 0 0 0\n");

    auto result = analyzer.getSystemDiskIoStats();
    ASSERT_TRUE(result.has_value());
    const auto& devices = result.value();
    ASSERT_EQ(devices.size(), 1U);
    const DiskIoDeviceStats& dev = devices[0];
    EXPECT_EQ(dev.deviceName, "sda");
    EXPECT_EQ(dev.readsCompleted, 1000U);
    EXPECT_EQ(dev.readsMerged, 200U);
    EXPECT_EQ(dev.sectorsRead, 50000U);
    EXPECT_EQ(dev.readTimeMs, 1200U);
    EXPECT_EQ(dev.writesCompleted, 500U);
    EXPECT_EQ(dev.writesMerged, 100U);
    EXPECT_EQ(dev.sectorsWritten, 20000U);
    EXPECT_EQ(dev.writeTimeMs, 600U);
    EXPECT_EQ(dev.ioProgressMs, 0U);
    EXPECT_EQ(dev.ioWeightedTimeMs, 800U);
}

TEST_F(GetSystemDiskIoStatsTest, ParsesMultipleDevices) {
    mockProc->createFileAt("diskstats",
        "   8   0 sda 1000 200 50000 1200 500 100 20000 600 0 800 1800 0 0 0 0 0 0\n"
        "   8   1 sda1 100 20 5000 120 50 10 2000 60 0 80 180 0 0 0 0 0 0\n");

    auto result = analyzer.getSystemDiskIoStats();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 2U);
    EXPECT_EQ(result.value()[0].deviceName, "sda");
    EXPECT_EQ(result.value()[1].deviceName, "sda1");
}

TEST_F(GetSystemDiskIoStatsTest, MissingDiskstatsReturnsError) {
    auto result = analyzer.getSystemDiskIoStats();
    EXPECT_FALSE(result.has_value());
}

TEST(SystemDiskIoRatesTest, ReturnsNonNegativeRates) {
    ProcessAnalyzer analyzer("/proc");
    auto result = analyzer.getSystemDiskIoRates(std::chrono::milliseconds(1));
    ASSERT_TRUE(result.has_value());
    for (const auto& r : *result) {
        EXPECT_FALSE(r.deviceName.empty());
        EXPECT_GE(r.readsPerSec,          0.0);
        EXPECT_GE(r.writesPerSec,         0.0);
        EXPECT_GE(r.sectorsReadPerSec,    0.0);
        EXPECT_GE(r.sectorsWrittenPerSec, 0.0);
    }
}

TEST(SystemDiskIoRatesTest, MissingDiskstatsReturnsError) {
    MockProc mockProc("mock_proc_disk_io_rates_absent");
    ProcessAnalyzer analyzer(mockProc.getPath());
    auto result = analyzer.getSystemDiskIoRates(std::chrono::milliseconds(1));
    EXPECT_FALSE(result.has_value());
}
