// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <span>
#include <vector>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

namespace {
    constexpr int kCleanupRetryCount = 3;
    constexpr int kCleanupRetryDelayMs = 50;
    const std::vector<std::byte> kTestData = {std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}};
    const std::vector<std::byte> kAppendData = {std::byte{0x00}, std::byte{0xFF}};
}

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
        if (ec) {
            // Retry a few times if cleanup failed (e.g. Windows file locking)
            for (int i = 0; i < kCleanupRetryCount; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(kCleanupRetryDelayMs));
                fs::remove_all(testDir, ec);
                if (!ec) break;
            }
        }
    }

    bool canCreateSymlinks() {
        std::error_code ec;
        auto target = testDir / "symlink_test_target";
        auto link = testDir / "symlink_test_link";
        
        // Create target if not exists
        if (!fs::exists(target)) {
            std::ofstream(target) << "test";
        }
        
        fs::create_symlink(target, link, ec);
        if (!ec) {
            fs::remove(link, ec);
            return true;
        }
        return false;
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

    // Test with a symlink (if supported)
    if (canCreateSymlinks()) {
        auto symlinkPath = testDir / "link_to_A";
        fs::create_symlink(fileAPath, symlinkPath);
        auto result3 = utils::canonicalPath(symlinkPath);
        ASSERT_TRUE(result3.has_value());
        EXPECT_EQ(result3.value(), fs::canonical(fileAPath));
    }

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
    
    // 5. Cross-drive (Windows-ish check)
    // We can't easily simulate drives on Linux without mount points, 
    // but we can check the implementation behavior if it were possible.
    // Ideally utils::makeRelative handles root differences.
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
    if (canCreateSymlinks()) {
        auto symlinkPath = testDir / "link_to_A";
        fs::create_symlink(fileAPath, symlinkPath);
        EXPECT_TRUE(utils::pathsEquivalent(fileAPath, symlinkPath));
    }

    // 4. Two different files
    EXPECT_FALSE(utils::pathsEquivalent(fileAPath, fileBPath));

    // 5. One or both paths being non-existent
    auto nonExistentPath = testDir / "nonexistent.txt";
    EXPECT_FALSE(utils::pathsEquivalent(fileAPath, nonExistentPath));
    EXPECT_FALSE(utils::pathsEquivalent(nonExistentPath, fileAPath));
    EXPECT_FALSE(utils::pathsEquivalent(nonExistentPath, testDir / "another_nonexistent.txt"));
}

TEST_F(UtilsNewApiTest, SymlinkManagement) {
    if (!canCreateSymlinks()) {
        GTEST_SKIP() << "Symlink creation not supported (insufficient privileges or filesystem support).";
    }

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

    // 6. Dangling symlink
    auto danglingLink = testDir / "dangling";
    fs::create_symlink(testDir / "nonexistent_target", danglingLink);
    EXPECT_TRUE(utils::isSymlink(danglingLink));
    EXPECT_FALSE(fs::exists(danglingLink)); // Target doesn't exist
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
    
    // Test actual write capability check (Fix 2.2)
    {
        std::ofstream os(testFile, std::ios::app);
        EXPECT_TRUE(os.good());
    }

    #ifndef _WIN32
    EXPECT_FALSE(utils::isExecutable(testFile));
    #endif

    // Make read-only
    ASSERT_TRUE(utils::setPermissions(testFile, fs::perms::owner_read).has_value());
    EXPECT_TRUE(utils::isReadable(testFile));
    EXPECT_FALSE(utils::isWritable(testFile));
    
    // Verify actual write failure
    {
        std::ofstream os(testFile, std::ios::app);
        EXPECT_FALSE(os.good());
    }

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

// --------------------------------------------------------------------------
// New Tests for Missing Implementations
// --------------------------------------------------------------------------

TEST_F(UtilsNewApiTest, ReadWriteBinaryFile) {
    auto binaryFile = testDir / "binary.dat";
    std::vector<std::byte> data = kTestData;

    // Write
    auto writeResult = utils::writeBinaryFile(binaryFile, data);
    ASSERT_TRUE(writeResult.has_value());
    ASSERT_TRUE(fs::exists(binaryFile));
    EXPECT_EQ(fs::file_size(binaryFile), 4);

    // Read
    auto readResult = utils::readBinaryFile(binaryFile);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), data);

    // Append
    std::vector<std::byte> moreData = kAppendData;
    auto appendResult = utils::appendToBinaryFile(binaryFile, moreData);
    ASSERT_TRUE(appendResult.has_value());
    
    // Read again
    auto readResult2 = utils::readBinaryFile(binaryFile);
    ASSERT_TRUE(readResult2.has_value());
    EXPECT_EQ(readResult2.value().size(), 6);
    EXPECT_EQ(readResult2.value()[4], kAppendData[0]);
    EXPECT_EQ(readResult2.value()[5], kAppendData[1]);
}

TEST_F(UtilsNewApiTest, WriteAtomic) {
    auto path = testDir / "atomic.txt";
    std::string content = "atomic content";
    
    auto result = utils::writeTextFileAtomic(path, content);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(fs::exists(path));
    
    auto readRes = utils::readTextFile(path);
    ASSERT_TRUE(readRes.has_value());
    EXPECT_EQ(readRes.value(), content);

    // Binary atomic
    auto binPath = testDir / "atomic.bin";
    std::vector<std::byte> binContent = {std::byte{1}, std::byte{2}};
    auto binResult = utils::writeBinaryFileAtomic(binPath, binContent);
    ASSERT_TRUE(binResult.has_value());
    EXPECT_TRUE(fs::exists(binPath));
}

TEST_F(UtilsNewApiTest, TemporaryFiles) {
    auto tempFile = utils::createTemporaryFile("test_prefix_", ".tmp");
    ASSERT_TRUE(tempFile.has_value());
    EXPECT_TRUE(fs::exists(tempFile.value()));
    EXPECT_TRUE(fs::is_regular_file(tempFile.value()));
    
    std::string filename = tempFile.value().filename().string();
    EXPECT_TRUE(filename.find("test_prefix_") == 0);
    EXPECT_TRUE(filename.find(".tmp") != std::string::npos);

    auto tempDir = utils::createTemporaryDirectory("test_dir_");
    ASSERT_TRUE(tempDir.has_value());
    EXPECT_TRUE(fs::exists(tempDir.value()));
    EXPECT_TRUE(fs::is_directory(tempDir.value()));
    
    std::string dirname = tempDir.value().filename().string();
    EXPECT_TRUE(dirname.find("test_dir_") == 0);

    // Cleanup
    fs::remove(tempFile.value());
    fs::remove_all(tempDir.value());
}

TEST_F(UtilsNewApiTest, TraverseDirectory) {
    // Setup directory structure
    // root/
    //   file1.txt
    //   sub1/
    //     file2.txt
    //   sub2/
    //     sub3/
    //       file3.txt
    
    auto sub1 = testDir / "sub1";
    auto sub2 = testDir / "sub2";
    auto sub3 = sub2 / "sub3";
    fs::create_directories(sub1);
    fs::create_directories(sub3);
    
    std::ofstream(testDir / "file1.txt") << "1";
    std::ofstream(sub1 / "file2.txt") << "2";
    std::ofstream(sub3 / "file3.txt") << "3";

    // 1. Recursive traversal
    std::vector<fs::path> visited;
    utils::TraversalOptions opts;
    opts.recursive = true;
    
    EXPECT_TRUE(utils::traverseDirectory(testDir, [&](const fs::directory_entry& entry) {
        visited.push_back(entry.path());
        return utils::TraversalControl::Continue;
    }, opts).has_value());
    
    // Expect 5 entries: file1, sub1, file2, sub2, sub3, file3 (order depends on OS)
    // Actually we have: file1.txt, sub1, sub2. Inside sub1: file2.txt. Inside sub2: sub3. Inside sub3: file3.txt.
    // Total: 3 (files) + 3 (dirs) = 6 entries (excluding root itself)
    EXPECT_GE(visited.size(), 6);

    // 2. Non-recursive
    visited.clear();
    opts.recursive = false;
    EXPECT_TRUE(utils::traverseDirectory(testDir, [&](const fs::directory_entry& entry) {
        visited.push_back(entry.path());
        return utils::TraversalControl::Continue;
    }, opts).has_value());
    
    // Expect file1, sub1, sub2
    EXPECT_EQ(visited.size(), 3);

    // 3. Stop control
    int count = 0;
    EXPECT_TRUE(utils::traverseDirectory(testDir, [&](const fs::directory_entry&) {
        count++;
        return utils::TraversalControl::stop;
    }, opts).has_value());
    EXPECT_EQ(count, 1);

    // 4. SkipDir control
    visited.clear();
    opts.recursive = true;
    EXPECT_TRUE(utils::traverseDirectory(testDir, [&](const fs::directory_entry& entry) {
        visited.push_back(entry.path());
        if (entry.is_directory() && entry.path().filename() == "sub1") {
            return utils::TraversalControl::skipDir;
        }
        return utils::TraversalControl::Continue;
    }, opts).has_value());
    
    // Should visit sub1 (the entry itself) but NOT file2.txt inside it
    bool visitedSub1 = false;
    bool visitedFile2 = false;
    for(const auto& p : visited) {
        if (p.filename() == "sub1") visitedSub1 = true;
        if (p.filename() == "file2.txt") visitedFile2 = true;
    }
    EXPECT_TRUE(visitedSub1); // "sub1" itself is visited
    EXPECT_FALSE(visitedFile2); // its children skipped
}
