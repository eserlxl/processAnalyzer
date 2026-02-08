// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

// Base fixture for tests requiring a temporary directory
class TempDirTest : public ::testing::Test {
protected:
    fs::path testDir;

    void SetUp() override {
        // Use a robust naming convention for temp directories
        const ::testing::TestInfo* const testInfo =
            ::testing::UnitTest::GetInstance()->current_test_info();
        testDir = fs::temp_directory_path() / (std::string(testInfo->test_suite_name()) + "_" + testInfo->name());
        
        // Clean up any previous run debris
        std::error_code ec;
        fs::remove_all(testDir, ec); 
        
        fs::create_directories(testDir);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(testDir, ec);
    }
};

// Test fixture for new API tests
class UtilsNewApiTest : public TempDirTest {
};

// Test fixture for permission tests
class UtilsPermissionsTest : public TempDirTest {
protected:
    fs::path testFile;

    void SetUp() override {
        TempDirTest::SetUp();
        testFile = testDir / "testfile.txt";
        std::ofstream(testFile) << "content";
    }
};

TEST_F(UtilsNewApiTest, CanonicalPath) {
    auto fileAPath = testDir / "fileA.txt";
    std::ofstream(fileAPath) << "content";

    // Test with a simple path
    auto result1 = utils::canonicalPath(fileAPath);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), fs::canonical(fileAPath));

    // Test with . and ..
    auto pathWithDots = testDir / ".." / testDir.filename() / "fileA.txt";
    auto result2 = utils::canonicalPath(pathWithDots);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), fs::canonical(fileAPath));

    // Test with a symlink
    auto symlinkPath = testDir / "link_to_A";
    fs::create_symlink(fileAPath, symlinkPath);
    auto result3 = utils::canonicalPath(symlinkPath);
    ASSERT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value(), fs::canonical(fileAPath));

    // Test with non-existent path
    auto nonExistentPath = testDir / "nonexistent.txt";
    auto result4 = utils::canonicalPath(nonExistentPath);
    ASSERT_FALSE(result4.has_value());
}

TEST_F(UtilsNewApiTest, MakeRelative) {
    auto base = testDir / "a" / "b";
    auto path = testDir / "a" / "b" / "c" / "file.txt";
    fs::create_directories(base);

    // 1. Simple case
    auto result1 = utils::makeRelative(path, base);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), fs::path("c") / "file.txt");

    // 2. Path outside base
    auto otherPath = testDir / "x" / "y";
    auto result2 = utils::makeRelative(otherPath, base);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), fs::path("..") / ".." / "x" / "y");

    // 3. Same paths
    auto result3 = utils::makeRelative(base, base);
    ASSERT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value(), ".");

    // 4. Not a prefix
    auto path4 = testDir / "a" / "d";
    auto result4 = utils::makeRelative(path4, base);
    ASSERT_TRUE(result4.has_value());
    EXPECT_EQ(result4.value(), fs::path("..") / "d");
}

TEST_F(UtilsNewApiTest, PathsEquivalent) {
    auto fileAPath = testDir / "fileA.txt";
    std::ofstream(fileAPath) << "content";
    auto fileBPath = testDir / "fileB.txt";
    std::ofstream(fileBPath) << "content";

    // 1. Two identical paths
    EXPECT_TRUE(utils::pathsEquivalent(fileAPath, fileAPath));

    // 2. Two different paths pointing to the same file
    auto pathWithDots = testDir / ".." / testDir.filename() / "fileA.txt";
    EXPECT_TRUE(utils::pathsEquivalent(fileAPath, pathWithDots));

    // 3. One path being a symlink to the other
    auto symlinkPath = testDir / "link_to_A";
    fs::create_symlink(fileAPath, symlinkPath);
    EXPECT_TRUE(utils::pathsEquivalent(fileAPath, symlinkPath));

    // 4. Two different files
    EXPECT_FALSE(utils::pathsEquivalent(fileAPath, fileBPath));

    // 5. One or both paths being non-existent
    auto nonExistentPath = testDir / "nonexistent.txt";
    EXPECT_FALSE(utils::pathsEquivalent(fileAPath, nonExistentPath));
    EXPECT_FALSE(utils::pathsEquivalent(nonExistentPath, fileAPath));
    EXPECT_FALSE(utils::pathsEquivalent(nonExistentPath, testDir / "another_nonexistent.txt"));
}

TEST_F(UtilsNewApiTest, SymlinkManagement) {
    auto targetPath = testDir / "target.txt";
    std::ofstream(targetPath) << "symlink target";

    auto linkPath = testDir / "link.txt";

    // 1. Create symlink
    auto createResult = utils::createSymlink(targetPath, linkPath);
    ASSERT_TRUE(createResult.has_value());

    // 2. Check if it's a symlink
    EXPECT_TRUE(utils::isSymlink(linkPath));
    EXPECT_TRUE(fs::exists(linkPath));
    
    // 3. Read the symlink
    auto readResult = utils::readSymlink(linkPath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), targetPath);

    // 4. Check isSymlink on non-symlinks
    EXPECT_FALSE(utils::isSymlink(targetPath)); // It's a regular file
    EXPECT_FALSE(utils::isSymlink(testDir / "nonexistent"));

    // 5. Error conditions
    // a. Create a link that already exists
    auto createAgainResult = utils::createSymlink(targetPath, linkPath);
    EXPECT_FALSE(createAgainResult.has_value());

    // b. Read a non-link
    auto readNonLinkResult = utils::readSymlink(targetPath);
    EXPECT_FALSE(readNonLinkResult.has_value());
}

TEST_F(UtilsPermissionsTest, GetAndSetPermissions) {
    // 1. Get initial permissions
    auto initialPermsResult = utils::getPermissions(testFile);
    ASSERT_TRUE(initialPermsResult.has_value());
    
    // 2. Set new, specific permissions
    auto newPerms = fs::perms::owner_read | fs::perms::owner_write;
    auto setResult = utils::setPermissions(testFile, newPerms);
    ASSERT_TRUE(setResult.has_value());

    // 3. Get again and verify
    auto finalPermsResult = utils::getPermissions(testFile);
    ASSERT_TRUE(finalPermsResult.has_value());
    EXPECT_EQ(finalPermsResult.value(), newPerms);

    // 4. Test on non-existent file
    auto nonExistentPath = testDir / "nonexistent.txt";
    auto getNonExistentResult = utils::getPermissions(nonExistentPath);
    EXPECT_FALSE(getNonExistentResult.has_value());
}

TEST_F(UtilsPermissionsTest, AddAndRemovePermissions) {
    // 1. Set known initial state (owner read/write)
    auto initialPerms = fs::perms::owner_read | fs::perms::owner_write;
    ASSERT_TRUE(utils::setPermissions(testFile, initialPerms).has_value());

    // 2. Add owner execute permission
    auto addPerms = fs::perms::owner_exec;
    auto addResult = utils::addPermissions(testFile, addPerms);
    ASSERT_TRUE(addResult.has_value());

    // 3. Verify owner execute is set and others are unchanged
    auto permsAfterAdd = utils::getPermissions(testFile);
    ASSERT_TRUE(permsAfterAdd.has_value());
    EXPECT_EQ(permsAfterAdd.value(), initialPerms | addPerms);

    // 4. Remove owner write permission
    auto removePerms = fs::perms::owner_write;
    auto removeResult = utils::removePermissions(testFile, removePerms);
    ASSERT_TRUE(removeResult.has_value());

    // 5. Verify owner write is gone and others are unchanged
    auto permsAfterRemove = utils::getPermissions(testFile);
    ASSERT_TRUE(permsAfterRemove.has_value());
    EXPECT_EQ(permsAfterRemove.value(), fs::perms::owner_read | fs::perms::owner_exec);
}

TEST_F(UtilsPermissionsTest, ReadWriteExecutableChecks) {
    // Test with default permissions (usually read/write for owner)
    EXPECT_TRUE(utils::isReadable(testFile));
    EXPECT_TRUE(utils::isWritable(testFile));
    // Executable is not usually set by default
    #ifndef _WIN32
    EXPECT_FALSE(utils::isExecutable(testFile));
    #endif

    // Make read-only
    ASSERT_TRUE(utils::setPermissions(testFile, fs::perms::owner_read).has_value());
    EXPECT_TRUE(utils::isReadable(testFile));
    EXPECT_FALSE(utils::isWritable(testFile));
    #ifndef _WIN32
    EXPECT_FALSE(utils::isExecutable(testFile));
    #endif
    
    // Make executable
    #ifndef _WIN32
    ASSERT_TRUE(utils::addPermissions(testFile, fs::perms::owner_exec).has_value());
    EXPECT_TRUE(utils::isExecutable(testFile));
    #endif
}

TEST_F(UtilsPermissionsTest, Chown) {
    auto chownResult = utils::chown(testFile, "user", "group");
    ASSERT_FALSE(chownResult.has_value());
    EXPECT_EQ(chownResult.error(), utils::make_error_code(utils::UtilsError::unsupportedOperation));
}
