// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "analyzer/core.h"
#include "analyzer/system_model.h"
#include "utils/testing_framework.h"

#include <filesystem>
#include <memory>
#include <string>

class GetSystemDiskUsageTest : public ::testing::Test {
protected:
    ProcessAnalyzer analyzer;
    std::filesystem::path originalProcPath;
    std::unique_ptr<MockProc> mockProc;

    GetSystemDiskUsageTest() : analyzer("/proc") {}

    void SetUp() override {
        mockProc = std::make_unique<MockProc>("mock_proc_diskusage_test");
        originalProcPath = analyzer.getProcPath();
        analyzer.setProcPath(mockProc->getPath());
    }

    void TearDown() override {
        mockProc.reset();
        analyzer.setProcPath(originalProcPath);
    }
};

TEST_F(GetSystemDiskUsageTest, ParsesRealMountPoint) {
    // Use mockProc->getPath() as the mount point — it's a real directory statvfs can query.
    std::string mountPoint = mockProc->getPath();
    std::string mountsContent =
        "/dev/sda1 " + mountPoint + " ext4 rw,relatime 0 0\n";
    mockProc->createFileAt("mounts", mountsContent);

    auto result = analyzer.getSystemDiskUsage();
    ASSERT_TRUE(result.has_value());
    const auto& mounts = result.value();
    ASSERT_FALSE(mounts.empty());
    const MountPointInfo& mp = mounts[0];
    EXPECT_EQ(mp.device, "/dev/sda1");
    EXPECT_EQ(mp.mountPoint, mountPoint);
    EXPECT_EQ(mp.filesystemType, "ext4");
    EXPECT_GT(mp.totalSpaceBytes, 0ULL);
}

TEST_F(GetSystemDiskUsageTest, SkipsPseudoFilesystems) {
    std::string mountPoint = mockProc->getPath();
    std::string mountsContent =
        "proc /proc proc rw 0 0\n"
        "sysfs /sys sysfs rw 0 0\n"
        "tmpfs /run tmpfs rw 0 0\n"
        "/dev/sda1 " + mountPoint + " ext4 rw,relatime 0 0\n";
    mockProc->createFileAt("mounts", mountsContent);

    auto result = analyzer.getSystemDiskUsage();
    ASSERT_TRUE(result.has_value());
    const auto& mounts = result.value();
    ASSERT_EQ(mounts.size(), 1U);
    EXPECT_EQ(mounts[0].filesystemType, "ext4");
}

TEST_F(GetSystemDiskUsageTest, SkipsBinfmtMiscAndFuseEntries) {
    std::string mountPoint = mockProc->getPath();
    std::string mountsContent =
        "binfmt_misc /proc/sys/fs/binfmt_misc binfmt_misc rw 0 0\n"
        "portal /home/user/.cache/doc fuse.portal rw 0 0\n"
        "gvfsd-fuse /home/user/.gvfs fuse.gvfsd-fuse rw 0 0\n"
        "overlay overlay overlay rw 0 0\n"
        "/dev/sda1 " + mountPoint + " ext4 rw,relatime 0 0\n";
    mockProc->createFileAt("mounts", mountsContent);

    auto result = analyzer.getSystemDiskUsage();
    ASSERT_TRUE(result.has_value());
    const auto& mounts = result.value();
    // Only the ext4 entry (with real statvfs-queryable mount point) should survive.
    ASSERT_EQ(mounts.size(), 1U);
    EXPECT_EQ(mounts[0].filesystemType, "ext4");
}

TEST_F(GetSystemDiskUsageTest, SkipsNonExistentMountPoints) {
    // A mount entry whose mount point does not exist on this system will cause
    // statvfs() to fail, so getSystemDiskUsage() silently skips it.
    std::string mountsContent =
        "/dev/sda99 /nonexistent/mount/point ext4 rw 0 0\n";
    mockProc->createFileAt("mounts", mountsContent);

    auto result = analyzer.getSystemDiskUsage();
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST_F(GetSystemDiskUsageTest, MissingMountsReturnsError) {
    auto result = analyzer.getSystemDiskUsage();
    EXPECT_FALSE(result.has_value());
}
