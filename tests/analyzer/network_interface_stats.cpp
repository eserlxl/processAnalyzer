// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/network_model.h"
#include "analyzer/system_model.h"
#include "utils/testing_framework.h" // For MockProc

#include <vector>
#include <string>
#include <filesystem>
#include <memory>
#include <algorithm>
#include <cstdint>

// Test suite for ProcessAnalyzer::getNetworkInterfaceStats
class GetNetworkInterfaceStatsTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    GetNetworkInterfaceStatsTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_net_stats_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
        mockProc->createDirectoryAt("net");
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetNetworkInterfaceStatsTest, ParsesValidDevFile) {
    std::string mockDevContent = 
        "Inter-|   Receive                                                |  Transmit\n"
        " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
        "    lo: 16186026  163821    0    0    0     0          0         0 16186026  163821    0    0    0     0       0          0\n"
        "  eth0: 12345678    1000    1    2    3     4          5         6 87654321    2000    7    8    9    10      11          12\n";
    
    mockProc->createFileAt("net/dev", mockDevContent);

    auto statsResult = analyzer.getNetworkInterfaceStats();
    ASSERT_TRUE(statsResult.has_value());
    const auto& stats = statsResult.value();

    ASSERT_EQ(stats.size(), 2);
    
    // Check lo
    const auto& lo = stats[0];
    EXPECT_EQ(lo.interfaceName, "lo");
    EXPECT_EQ(lo.rxBytes, 16186026);
    EXPECT_EQ(lo.rxPackets, 163821);
    EXPECT_EQ(lo.txBytes, 16186026);
    
    // Check eth0
    const auto& eth0 = stats[1];
    EXPECT_EQ(eth0.interfaceName, "eth0");
    EXPECT_EQ(eth0.rxBytes, 12345678);
    EXPECT_EQ(eth0.rxPackets, 1000);
    EXPECT_EQ(eth0.rxErrors, 1);
    EXPECT_EQ(eth0.rxDropped, 2);
    EXPECT_EQ(eth0.txBytes, 87654321);
    EXPECT_EQ(eth0.txPackets, 2000);
    EXPECT_EQ(eth0.txErrors, 7);
    EXPECT_EQ(eth0.txDropped, 8);
}

TEST_F(GetNetworkInterfaceStatsTest, HandlesMissingFile) {
    auto statsResult = analyzer.getNetworkInterfaceStats();
    ASSERT_FALSE(statsResult.has_value());
    EXPECT_EQ(statsResult.error().value(), static_cast<int>(utils::UtilsError::fileNotFound));
}

TEST_F(GetNetworkInterfaceStatsTest, HandlesEmptyFile) {
    mockProc->createFileAt("net/dev", "");
    auto statsResult = analyzer.getNetworkInterfaceStats();
    ASSERT_FALSE(statsResult.has_value());
    // Expect analyzerParsingError because it's empty
    EXPECT_EQ(statsResult.error().value(), static_cast<int>(utils::UtilsError::analyzerParsingError)); 
}
