// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/filesystem_attributes.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#ifdef __linux__
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#endif

namespace fs = std::filesystem;

namespace {
    constexpr int cleanupRetryCount = 3;
    constexpr int cleanupRetryDelayMs = 50;

    class TempDirTest : public ::testing::Test {
    protected:
        fs::path testDir;

        void SetUp() override {
            const ::testing::TestInfo* const testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
            testDir = fs::temp_directory_path() / (std::string(testInfo->test_suite_name()) + "_" + testInfo->name());
            std::error_code ec;
            fs::remove_all(testDir, ec); 
            fs::create_directories(testDir, ec);
        }

        void TearDown() override {
            std::error_code ec;
            fs::remove_all(testDir, ec);
            if (ec) {
                for (int i = 0; i < cleanupRetryCount; ++i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cleanupRetryDelayMs));
                    fs::remove_all(testDir, ec);
                    if (!ec) break;
                }
            }
        }
    };
}

class FilesystemAttributesTest : public TempDirTest {
protected:
    fs::path testFile;
    
    void SetUp() override {
        TempDirTest::SetUp();
        testFile = testDir / "testfile.txt";
        std::ofstream(testFile) << "content";
    }
};

TEST_F(FilesystemAttributesTest, GetAndSetPermissions) {
    auto initialPermsResult = utils::getPermissions(testFile);
    ASSERT_TRUE(initialPermsResult.has_value());
    
    auto newPerms = fs::perms::owner_read | fs::perms::owner_write;
    auto setResult = utils::setPermissions(testFile, newPerms);
    ASSERT_TRUE(setResult.has_value());

    auto finalPermsResult = utils::getPermissions(testFile);
    ASSERT_TRUE(finalPermsResult.has_value());
    EXPECT_EQ(finalPermsResult.value(), newPerms);

    auto nonExistentPath = testDir / "nonexistent.txt";
    auto getNonExistentResult = utils::getPermissions(nonExistentPath);
    EXPECT_FALSE(getNonExistentResult.has_value());
}

TEST_F(FilesystemAttributesTest, AddAndRemovePermissions) {
    auto initialPerms = fs::perms::owner_read | fs::perms::owner_write;
    ASSERT_TRUE(utils::setPermissions(testFile, initialPerms).has_value());

    auto addPerms = fs::perms::owner_exec;
    auto addResult = utils::addPermissions(testFile, addPerms);
    ASSERT_TRUE(addResult.has_value());

    auto permsAfterAdd = utils::getPermissions(testFile);
    ASSERT_TRUE(permsAfterAdd.has_value());
    EXPECT_EQ(permsAfterAdd.value(), initialPerms | addPerms);

    auto removePerms = fs::perms::owner_write;
    auto removeResult = utils::removePermissions(testFile, removePerms);
    ASSERT_TRUE(removeResult.has_value());

    auto permsAfterRemove = utils::getPermissions(testFile);
    ASSERT_TRUE(permsAfterRemove.has_value());
    EXPECT_EQ(permsAfterRemove.value(), fs::perms::owner_read | fs::perms::owner_exec);
}

TEST_F(FilesystemAttributesTest, ReadWriteExecutableChecks) {
    EXPECT_TRUE(utils::isReadable(testFile).value());
    EXPECT_TRUE(utils::isWritable(testFile).value());
    
    #ifndef _WIN32
    EXPECT_FALSE(utils::isExecutable(testFile).value());
    #endif

    ASSERT_TRUE(utils::setPermissions(testFile, fs::perms::owner_read).has_value());
    EXPECT_TRUE(utils::isReadable(testFile).value());
    EXPECT_FALSE(utils::isWritable(testFile).value());

    #ifndef _WIN32
    EXPECT_FALSE(utils::isExecutable(testFile).value());
    ASSERT_TRUE(utils::addPermissions(testFile, fs::perms::owner_exec).has_value());
    EXPECT_TRUE(utils::isExecutable(testFile).value());
    #endif
}

#ifdef __linux__
TEST_F(FilesystemAttributesTest, Chown) {
    auto result = utils::chown(testFile, "nonexistentuser_12345", "nonexistentgroup_12345");
    ASSERT_FALSE(result.has_value());

    if (getuid() != 0) {
        auto nonRootResult = utils::chown(testFile, "root", "");
        EXPECT_FALSE(nonRootResult.has_value());
    }
}
#endif

TEST_F(FilesystemAttributesTest, ExistsIsFileIsDirectory) {
    auto dirPath = testDir / "subdir";
    fs::create_directory(dirPath);

    auto existsFile = utils::exists(testFile);
    ASSERT_TRUE(existsFile.has_value());
    EXPECT_TRUE(existsFile.value());

    auto isFileResult = utils::isFile(testFile);
    ASSERT_TRUE(isFileResult.has_value());
    EXPECT_TRUE(isFileResult.value());

    auto isDirResult = utils::isDirectory(testFile);
    ASSERT_TRUE(isDirResult.has_value());
    EXPECT_FALSE(isDirResult.value());
    
    auto isDirResult2 = utils::isDirectory(dirPath);
    ASSERT_TRUE(isDirResult2.has_value());
    EXPECT_TRUE(isDirResult2.value());
}

TEST_F(FilesystemAttributesTest, ExistsErrorPropagation) {
    fs::path inaccessibleDir = testDir / "inaccessible";
    fs::create_directory(inaccessibleDir);
    fs::path childFile = inaccessibleDir / "child.txt";
    std::ofstream(childFile) << "content";

    fs::permissions(inaccessibleDir, fs::perms::none);

    auto existsResult = utils::exists(childFile);
    ASSERT_FALSE(existsResult.has_value());

    fs::permissions(inaccessibleDir, fs::perms::all);
}

TEST_F(FilesystemAttributesTest, GetFileSize) {
    auto path = testDir / "size.txt";
    std::string content = "12345";
    std::ofstream(path) << content;
    
    auto result = utils::getFileSize(path);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), content.size());

    auto emptyPath = testDir / "empty.txt";
    std::ofstream(emptyPath) << "";
    result = utils::getFileSize(emptyPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 0);

    result = utils::getFileSize(testDir / "nonexistent");
    ASSERT_FALSE(result.has_value());

    result = utils::getFileSize(testDir);
    ASSERT_FALSE(result.has_value());
}

TEST_F(FilesystemAttributesTest, IsSymlink) {
    auto targetPath = testDir / "target.txt";
    std::ofstream(targetPath) << "symlink target";
    auto linkPath = testDir / "link.txt";

    std::error_code ec;
    fs::create_symlink(targetPath, linkPath, ec);
    if (ec) {
        GTEST_SKIP() << "Symlink creation not supported.";
    }

    auto isSymlinkResult = utils::isSymlink(linkPath);
    ASSERT_TRUE(isSymlinkResult.has_value());
    EXPECT_TRUE(isSymlinkResult.value());
    
    auto isNotSymlinkResult = utils::isSymlink(targetPath);
    ASSERT_TRUE(isNotSymlinkResult.has_value());
    EXPECT_FALSE(isNotSymlinkResult.value());
}