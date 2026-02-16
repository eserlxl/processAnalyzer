// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#include "utils/file.h"
#include "gtest/gtest.h"
#include "utils/core.h"
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
// Base fixture for tests requiring a temporary directory
class TempDirTest : public ::testing::Test {
protected:
    fs::path testDir;

    void SetUp() override {
        const ::testing::TestInfo* const testInfo =
            ::testing::UnitTest::GetInstance()->current_test_info();
        testDir = fs::temp_directory_path() / (std::string(testInfo->test_suite_name()) + "_" + testInfo->name());
        
        std::error_code ec;
        fs::remove_all(testDir, ec); 
        
        ASSERT_TRUE(utils::createDirectories(testDir));
    }
    
    void TearDown() override {
        std::error_code ec;
        fs::remove_all(testDir, ec);
    }
};
}

class FileExtendedTest : public TempDirTest {};

TEST_F(FileExtendedTest, Permissions) {
    auto path = testDir / "perms.txt";
    std::ofstream(path) << "test";

    // Set permissions
    auto setResult = utils::setPermissions(path, fs::perms::owner_read | fs::perms::group_read);
    ASSERT_TRUE(setResult.has_value());

    // Get and verify
    auto getResult = utils::getPermissions(path);
    ASSERT_TRUE(getResult.has_value());
    EXPECT_EQ(getResult.value() & fs::perms::owner_read, fs::perms::owner_read);
    EXPECT_EQ(getResult.value() & fs::perms::group_read, fs::perms::group_read);
    EXPECT_EQ(getResult.value() & fs::perms::owner_write, fs::perms::none);

    // Add permissions
    auto addResult = utils::addPermissions(path, fs::perms::owner_write);
    ASSERT_TRUE(addResult.has_value());
    getResult = utils::getPermissions(path);
    ASSERT_TRUE(getResult.has_value());
    EXPECT_NE(getResult.value() & fs::perms::owner_write, fs::perms::none);

    // Remove permissions
    auto removeResult = utils::removePermissions(path, fs::perms::owner_read);
    ASSERT_TRUE(removeResult.has_value());
    getResult = utils::getPermissions(path);
    ASSERT_TRUE(getResult.has_value());
    EXPECT_EQ(getResult.value() & fs::perms::owner_read, fs::perms::none);

    // Error case
    auto nonExistent = testDir / "nonexistent.txt";
    auto errResult = utils::getPermissions(nonExistent);
    ASSERT_FALSE(errResult.has_value());
}

#ifdef __linux__
TEST_F(FileExtendedTest, Chown) {
    auto path = testDir / "chown_test.txt";
    std::ofstream(path) << "test";

    // Test failure for non-existent users
    auto result = utils::chown(path, "nonexistentuser12345", "");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::invalidArgument));

    if (getuid() != 0) {
        // If not root, changing ownership to another user (even 'root') should fail.
        auto nonRootResult = utils::chown(path, "root", "");
        EXPECT_FALSE(nonRootResult.has_value());
        EXPECT_EQ(nonRootResult.error(), std::errc::operation_not_permitted);
    } else {
        // If root, test with a valid user/group that is not root.
        // For CI environments, 'nobody' and 'nogroup' are common.
        struct passwd* nobodyUser = getpwnam("nobody");
        struct group* nobodyGroup = getgrnam("nogroup");
        if (nobodyUser && nobodyGroup) {
            auto rootResult = utils::chown(path, "nobody", "nogroup");
            ASSERT_TRUE(rootResult.has_value());
            struct stat st;
            ASSERT_EQ(stat(path.c_str(), &st), 0);
            EXPECT_EQ(st.st_uid, nobodyUser->pw_uid);
            EXPECT_EQ(st.st_gid, nobodyGroup->gr_gid);
        }
    }
}
#endif

TEST_F(FileExtendedTest, ReadWriteExecutableStatus) {
    auto path = testDir / "status_test.txt";
    std::ofstream(path) << "test";

    ASSERT_TRUE(utils::setPermissions(path, fs::perms::owner_read | fs::perms::owner_write));
    auto readable = utils::isReadable(path);
    ASSERT_TRUE(readable.has_value());
    EXPECT_TRUE(readable.value());
    auto writable = utils::isWritable(path);
    ASSERT_TRUE(writable.has_value());
    EXPECT_TRUE(writable.value());
    auto executable = utils::isExecutable(path);
    ASSERT_TRUE(executable.has_value());
    EXPECT_FALSE(executable.value());

    ASSERT_TRUE(utils::setPermissions(path, fs::perms::owner_exec));
    readable = utils::isReadable(path);
    ASSERT_TRUE(readable.has_value());
    EXPECT_FALSE(readable.value());
    writable = utils::isWritable(path);
    ASSERT_TRUE(writable.has_value());
    EXPECT_FALSE(writable.value());
    executable = utils::isExecutable(path);
    ASSERT_TRUE(executable.has_value());
    EXPECT_TRUE(executable.value());

    // Non-existent file
    auto nonExistent = testDir / "nonexistent.txt";
    auto errResult = utils::isReadable(nonExistent);
    ASSERT_FALSE(errResult.has_value());
}

TEST_F(FileExtendedTest, Remove) {
    // Non-recursive remove of non-empty directory
    auto dir = testDir / "dir";
    ASSERT_TRUE(utils::createDirectories(dir));
    std::ofstream(dir / "file.txt") << "content";
    auto result = utils::remove(dir, false);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), std::errc::directory_not_empty);

    // Recursive remove
    auto resultRec = utils::remove(dir, true);
    ASSERT_TRUE(resultRec.has_value());
    EXPECT_FALSE(fs::exists(dir));
}

TEST_F(FileExtendedTest, TraverseDirectory) {
    // Setup directory structure
    auto root = testDir / "traverse";
    ASSERT_TRUE(utils::createDirectories(root / "dir1" / "subdir1"));
    ASSERT_TRUE(utils::createDirectories(root / "dir2"));
    std::ofstream(root / "root_file.txt") << "r";
    std::ofstream(root / "dir1" / "d1_file.txt") << "d1";
    std::ofstream(root / "dir1" / "subdir1" / "sd1_file.txt") << "sd1";
    std::ofstream(root / "dir2" / "d2_file.txt") << "d2";

    // Basic traversal
    std::vector<fs::path> visited;
    auto cb = [&](const fs::directory_entry& entry) {
        visited.push_back(entry.path());
        return utils::TraversalControl::Continue;
    };
    auto result = utils::traverseDirectory(root, cb);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(visited.size(), 7); // root, dir1, subdir1, root_file, d1_file, sd1_file, d2_file

    // Non-recursive
    visited.clear();
    utils::TraversalOptions optsNorec;
    optsNorec.recursive = false;
    result = utils::traverseDirectory(root, cb, optsNorec);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(visited.size(), 3); // dir1, dir2, root_file.txt

    // maxDepth
    visited.clear();
    utils::TraversalOptions optsDepth;
    optsDepth.maxDepth = 1;
    result = utils::traverseDirectory(root, cb, optsDepth);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(visited.size(), 6); 

    // skipDir
    visited.clear();
    auto cbSkip = [&](const fs::directory_entry& entry) {
        visited.push_back(entry.path());
        if (entry.path().filename() == "dir1") {
            return utils::TraversalControl::skipDir;
        }
        return utils::TraversalControl::Continue;
    };
    result = utils::traverseDirectory(root, cbSkip);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(visited.size(), 4); // root, dir1, root_file, dir2, d2_file (dir1's children are skipped)

    // stop
    visited.clear();
    size_t count = 0;
    auto cbStop = [&](const fs::directory_entry& entry) {
        visited.push_back(entry.path());
        count++;
        if (count == 3) {
            return utils::TraversalControl::stop;
        }
        return utils::TraversalControl::Continue;
    };
    result = utils::traverseDirectory(root, cbStop);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::traversalStopped));
    EXPECT_EQ(visited.size(), 3);
}

TEST_F(FileExtendedTest, ListDirectory) {
    auto dir = testDir / "list";
    ASSERT_TRUE(utils::createDirectories(dir));
    std::ofstream(dir / "f1.txt") << "f1";
    ASSERT_TRUE(utils::createDirectories(dir / "sub"));

    auto result = utils::listDirectory(dir);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().size(), 2);

    auto emptyDir = testDir / "empty";
    ASSERT_TRUE(utils::createDirectories(emptyDir));
    result = utils::listDirectory(emptyDir);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());

    // non-existent
    result = utils::listDirectory(testDir / "nonexistent");
    ASSERT_FALSE(result.has_value());
}

TEST_F(FileExtendedTest, CopyMoveFile) {
    auto src = testDir / "src.txt";
    auto dst = testDir / "dst.txt";
    std::string content = "copy me";
    std::ofstream(src) << content;

    // Copy
    auto copyResult = utils::copyFile(src, dst);
    ASSERT_TRUE(copyResult.has_value());
    ASSERT_TRUE(fs::exists(dst));
    auto readRes = utils::readTextFile(dst);
    ASSERT_TRUE(readRes);
    EXPECT_EQ(readRes.value(), content);


    // Move
    auto moveDst = testDir / "move.txt";
    auto moveResult = utils::moveFile(dst, moveDst);
    ASSERT_TRUE(moveResult.has_value());
    EXPECT_FALSE(fs::exists(dst));
    EXPECT_TRUE(fs::exists(moveDst));

    // Error case: source not found
    auto errResult = utils::copyFile(testDir / "nonexistent", dst);
    ASSERT_FALSE(errResult.has_value());
}

TEST_F(FileExtendedTest, GetFileSize) {
    auto path = testDir / "size.txt";
    std::string content = "12345";
    std::ofstream(path) << content;
    
    auto result = utils::getFileSize(path);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), content.size());

    // Empty file
    auto emptyPath = testDir / "empty.txt";
    std::ofstream(emptyPath) << "";
    result = utils::getFileSize(emptyPath);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 0);

    // Non-existent
    result = utils::getFileSize(testDir / "nonexistent");
    ASSERT_FALSE(result.has_value());

    // Directory
    result = utils::getFileSize(testDir);
    ASSERT_FALSE(result.has_value());
}
