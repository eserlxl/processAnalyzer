// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/process_model.h"
#include "utils/testing_framework.h" // For MockProc

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

class GetProcessMemoryMapsTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    static constexpr int kPid = 7000;
    static constexpr int kAbsentPid = 9999;
    static constexpr uint64_t kStart = 0x7f0000000000ULL;
    static constexpr uint64_t kEnd = 0x7f0000010000ULL;
    static constexpr uint64_t kOffset = 0x1000ULL;
    static constexpr uint64_t kInode = 1234;

    GetProcessMemoryMapsTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_maps_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
        mockProc->buildProcess(kPid).withName("mapproc").withParent(1).create();

        MockProc::ProcMapEntry fileMap;
        fileMap.addressRange = "7f0000000000-7f0000010000";
        fileMap.perms = "r-xp";
        fileMap.offset = kOffset;
        fileMap.dev = "08:01";
        fileMap.inode = kInode;
        fileMap.pathname = "/usr/bin/ls";

        MockProc::ProcMapEntry anonMap;
        anonMap.addressRange = "7f0000020000-7f0000021000";
        anonMap.perms = "rw-p";
        anonMap.offset = 0;
        anonMap.dev = "00:00";
        anonMap.inode = 0;
        // anonMap.pathname left empty

        mockProc->createMaps(kPid, {fileMap, anonMap});
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetProcessMemoryMapsTest, ParsesMapEntries) {
    auto result = analyzer.getProcessMemoryMaps(kPid);
    ASSERT_TRUE(result.has_value());
    const auto& maps = result.value();
    ASSERT_EQ(maps.size(), 2U);

    const MemoryMapInfo& fileMap = maps[0];
    EXPECT_EQ(fileMap.startAddress, kStart);
    EXPECT_EQ(fileMap.endAddress, kEnd);
    EXPECT_EQ(fileMap.permissions, "r-xp");
    EXPECT_EQ(fileMap.offset, kOffset);
    EXPECT_EQ(fileMap.device, "08:01");
    EXPECT_EQ(fileMap.inode, kInode);
    EXPECT_EQ(fileMap.pathname, "/usr/bin/ls");

    // The anonymous mapping has no pathname.
    EXPECT_TRUE(maps[1].pathname.empty());
}

TEST_F(GetProcessMemoryMapsTest, AbsentPidReturnsError) {
    auto result = analyzer.getProcessMemoryMaps(kAbsentPid);
    EXPECT_FALSE(result.has_value());
}
