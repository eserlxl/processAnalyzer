// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/system_model.h"
#include "utils/testing_framework.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

// Real-/proc smoke test: non-empty result, all rates finite and non-negative.
TEST(NetworkInterfaceRatesTest, ZeroDurationReturnsFiniteRates) {
    ProcessAnalyzer analyzer("/proc");
    auto result = analyzer.getNetworkInterfaceRates(std::chrono::milliseconds(0));
    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(result->empty());
    for (const auto& r : *result) {
        EXPECT_TRUE(std::isfinite(r.rxBytesPerSec));
        EXPECT_TRUE(std::isfinite(r.txBytesPerSec));
        EXPECT_TRUE(std::isfinite(r.rxPacketsPerSec));
        EXPECT_TRUE(std::isfinite(r.txPacketsPerSec));
        EXPECT_GE(r.rxBytesPerSec, 0.0);
        EXPECT_GE(r.txBytesPerSec, 0.0);
    }
}

// Mock-based: both snapshots read the same static file → all rates are 0.0.
TEST(NetworkInterfaceRatesTest, SameSnapshotGivesZeroRates) {
    MockProc mockProc("mock_proc_net_rates_same_snap");
    ProcessAnalyzer analyzer(mockProc.getPath());
    mockProc.createDirectoryAt("net");
    mockProc.createFileAt("net/dev",
        "Inter-|   Receive                                                |  Transmit\n"
        " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
        "    lo: 1000000    5000    0    0    0     0          0         0 1000000    5000    0    0    0     0       0          0\n");

    auto result = analyzer.getNetworkInterfaceRates(std::chrono::milliseconds(1));
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result->size(), 1ULL);
    EXPECT_EQ(result->at(0).interfaceName, "lo");
    EXPECT_DOUBLE_EQ(result->at(0).rxBytesPerSec,   0.0);
    EXPECT_DOUBLE_EQ(result->at(0).txBytesPerSec,   0.0);
    EXPECT_DOUBLE_EQ(result->at(0).rxPacketsPerSec, 0.0);
    EXPECT_DOUBLE_EQ(result->at(0).txPacketsPerSec, 0.0);
}

// The rates vector must have the same number of entries as getNetworkInterfaceStats
// (all interfaces present in both snapshots are included, none duplicated).
TEST(NetworkInterfaceRatesTest, ResultCountMatchesStatCount) {
    ProcessAnalyzer analyzer("/proc");
    auto statsResult = analyzer.getNetworkInterfaceStats();
    ASSERT_TRUE(statsResult.has_value());

    auto ratesResult = analyzer.getNetworkInterfaceRates(std::chrono::milliseconds(1));
    ASSERT_TRUE(ratesResult.has_value());

    EXPECT_EQ(ratesResult->size(), statsResult->size());
}

// Missing net/dev file → both getNetworkInterfaceStats calls fail → error propagated.
TEST(NetworkInterfaceRatesTest, MissingNetDevFileReturnsError) {
    MockProc mockProc("mock_proc_net_rates_no_dev");
    ProcessAnalyzer analyzer(mockProc.getPath());

    auto result = analyzer.getNetworkInterfaceRates(std::chrono::milliseconds(1));
    EXPECT_FALSE(result.has_value());
}
