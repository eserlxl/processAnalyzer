// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/Core.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <span>
#include <vector>
#include <thread>
#include <chrono>
#include <random>

namespace fs = std::filesystem;

// Helper function to generate a random string
std::string generateRandomString(size_t length) {
    const std::string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, static_cast<int>(characters.size() - 1));
    std::string randomString;
    for (size_t i = 0; i < length; ++i) {
        randomString += characters[distribution(generator)];
    }
    return randomString;
}

constexpr size_t kRandomNameLen = 10;

namespace {
    constexpr int kCleanupRetryCount = 3;
    constexpr int kCleanupRetryDelayMs = 50;
}

// Base fixture for tests requiring a temporary directory
class TempDirTest : public ::testing::Test {
protected:
    fs::path testDir;
    fs::path originalPath;

    void SetUp() override {
        // Use a robust naming convention for temp directories
            const ::testing::TestInfo* const testInfo =
                ::testing::UnitTest::GetInstance()->current_test_info();
            testDir = fs::temp_directory_path() / (std::string(testInfo->test_suite_name()) + "_" + testInfo->name());
            
            // Clean up any previous run debris
            std::error_code ec;
            fs::remove_all(testDir, ec); 
            
            fs::create_directories(testDir);
            originalPath = fs::current_path();
        }
        
        void TearDown() override {
            fs::current_path(originalPath);
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

// --- Path-related Error Handling Tests ---
TEST_F(UtilsNewApiTest, TraverseDirectoryTraversalStopped) {
    fs::path sub1 = testDir / "sub1";
    fs::create_directories(sub1);
    std::ofstream(testDir / "file1.txt") << "1";
    std::ofstream(sub1 / "file2.txt") << "2";

    utils::TraversalOptions opts;
    opts.recursive = true;
    
    int count = 0;
    auto result = utils::traverseDirectory(testDir, [&](const fs::directory_entry& entry) {
        count++;
        if (entry.path().filename() == "sub1") {
            return utils::TraversalControl::stop;
        }
        return utils::TraversalControl::Continue;
    }, opts);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::traversalStopped));
    // Expect count to be 2 (file1.txt and sub1 itself before stopping)
    EXPECT_EQ(count, 2); 
}

TEST_F(UtilsNewApiTest, ExistsIsFileIsDirectoryErrorPropagation) {
    fs::path inaccessibleDir = testDir / "inaccessible";
    fs::create_directory(inaccessibleDir);
    fs::path childFile = inaccessibleDir / "child.txt";
    std::ofstream(childFile) << "content";

    // Revoke permissions for the inaccessibleDir to simulate error
    fs::permissions(inaccessibleDir, fs::perms::none);

    // Test exists on child file in inaccessible directory
    auto existsResult = utils::exists(childFile);
    ASSERT_FALSE(existsResult.has_value());
    EXPECT_TRUE(existsResult.error() == std::errc::permission_denied); 

    // Test isFile on child file
    auto isFileResult = utils::isFile(childFile);
    ASSERT_FALSE(isFileResult.has_value());
    EXPECT_TRUE(isFileResult.error() == std::errc::permission_denied);

    // Test isDirectory on inaccessible directory (actually, child of inaccessible)
    auto isDirectoryResult = utils::isDirectory(childFile); // childFile is in inaccessibleDir
    ASSERT_FALSE(isDirectoryResult.has_value());
    EXPECT_TRUE(isDirectoryResult.error() == std::errc::permission_denied);

    // Restore permissions for cleanup
    fs::permissions(inaccessibleDir, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);
}

TEST_F(UtilsNewApiTest, CreateDirectoriesErrorHandling) {
    // Permission denied
    fs::path noWriteParent = testDir / "no_write_parent";
    fs::create_directory(noWriteParent);
    fs::permissions(noWriteParent, fs::perms::owner_read); // Make it read-only

    auto inaccessibleSubDir = noWriteParent / "new_dir";
    auto result = utils::createDirectories(inaccessibleSubDir);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::permissionDenied));

    // Restore permissions
    fs::permissions(noWriteParent, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);

    // Intermediate path component is a file
    fs::path fileAsIntermediateDir = testDir / "file_here";
    std::ofstream(fileAsIntermediateDir) << "content";

    auto pathToCreate = fileAsIntermediateDir / "sub_dir" / "another_sub";
    result = utils::createDirectories(pathToCreate);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), std::errc::invalid_argument); // Intermediate path component is a file

    fs::remove(fileAsIntermediateDir); // Cleanup
}

TEST_F(UtilsNewApiTest, RemoveErrorHandling) {
    // Non-existent file/directory
    auto nonExistent = testDir / "no_such_thing";
    auto result = utils::remove(nonExistent, false);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::fileNotFound));

    // Permission denied
    fs::path readOnlyDir = testDir / "read_only_dir";
    fs::create_directory(readOnlyDir);
    auto protectedFile = readOnlyDir / "protected.txt";
    std::ofstream(protectedFile) << "secret";
    
    // Revoke write permission from parent directory
    fs::permissions(readOnlyDir, fs::perms::owner_read | fs::perms::owner_exec, fs::perm_options::replace); 
    
    result = utils::remove(protectedFile, false);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::permissionDenied));

    // Restore permissions for cleanup
    fs::permissions(readOnlyDir, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);
    fs::remove_all(readOnlyDir); // Actual cleanup
}

TEST_F(UtilsNewApiTest, ListDirectoryErrorHandling) {
    // Non-existent directory
    auto nonExistentDir = testDir / "no_such_dir";
    auto result = utils::listDirectory(nonExistentDir);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::fileNotFound));

    // Path is a file
    auto fileInsteadOfDir = testDir / "my_file.txt";
    std::ofstream(fileInsteadOfDir) << "content";
    
    result = utils::listDirectory(fileInsteadOfDir);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::ioError)); // is_directory will fail

    // Test listDirectory on inaccessible directory
    fs::path inaccessibleDir = testDir / "inaccessible_for_list";
    fs::create_directory(inaccessibleDir);
    fs::permissions(inaccessibleDir, fs::perms::none);

    auto resultInacc = utils::listDirectory(inaccessibleDir);
    ASSERT_FALSE(resultInacc.has_value());
    EXPECT_TRUE(resultInacc.error() == std::errc::permission_denied);

    // Restore permissions
    fs::permissions(inaccessibleDir, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);
}

TEST_F(UtilsNewApiTest, CopyFileParentDirectoryCreation) {
    auto sourceFile = testDir / "source.txt";
    std::ofstream(sourceFile) << "original content";

    auto destinationDir = testDir / "new_parent" / "sub_folder";
    auto destinationFile = destinationDir / "destination.txt";

    // copyFile should create new_parent/sub_folder automatically
    auto result = utils::copyFile(sourceFile, destinationFile);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(fs::exists(destinationFile));
    EXPECT_TRUE(fs::exists(destinationDir));
    
    auto readResult = utils::readTextFile(destinationFile);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), "original content");

    // Test error propagation if createDirectories fails
    fs::path restrictedParent = testDir / "restricted_copy_target";
    fs::create_directory(restrictedParent);
    fs::permissions(restrictedParent, fs::perms::owner_read); // Make read-only

    auto inaccessibleDestination = restrictedParent / "sub_dir" / "file.txt";
    result = utils::copyFile(sourceFile, inaccessibleDestination);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::permissionDenied));

    // Restore permissions for cleanup
    fs::permissions(restrictedParent, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);
}
        
// --------------------------------------------------------------------------
// New Tests for Missing Implementations (Path Specific)
// --------------------------------------------------------------------------

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
}

TEST_F(UtilsNewApiTest, PathsEquivalent) {
    auto fileAPath = testDir / "fileA.txt";
    std::ofstream(fileAPath) << "content";
    auto fileBPath = testDir / "fileB.txt";
    std::ofstream(fileBPath) << "content";

    // 1. Two identical paths
    auto result1 = utils::pathsEquivalent(fileAPath, fileAPath);
    ASSERT_TRUE(result1.has_value());
    EXPECT_TRUE(result1.value());

    // 2. Two different paths pointing to the same file
    auto pathWithDots = testDir / ".." / testDir.filename() / "fileA.txt";
    auto result2 = utils::pathsEquivalent(fileAPath, pathWithDots);
    ASSERT_TRUE(result2.has_value());
    EXPECT_TRUE(result2.value());

    // 3. One path being a symlink to the other
    if (canCreateSymlinks()) {
        auto symlinkPath = testDir / "link_to_A";
        fs::create_symlink(fileAPath, symlinkPath);
        auto result3 = utils::pathsEquivalent(fileAPath, symlinkPath);
        ASSERT_TRUE(result3.has_value());
        EXPECT_TRUE(result3.value());
    }

    // 4. Two different files
    auto result4 = utils::pathsEquivalent(fileAPath, fileBPath);
    ASSERT_TRUE(result4.has_value());
    EXPECT_FALSE(result4.value());

    // 5. One or both paths being non-existent returns an error
    auto nonExistentPath = testDir / "nonexistent.txt";
    EXPECT_FALSE(utils::pathsEquivalent(fileAPath, nonExistentPath).has_value());
    EXPECT_FALSE(utils::pathsEquivalent(nonExistentPath, fileAPath).has_value());
    // equivalent(nonexistent, nonexistent) is platform-dependent, but usually fails.
    EXPECT_FALSE(utils::pathsEquivalent(nonExistentPath, testDir / "another_nonexistent.txt").has_value());
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
    auto isSymlinkResult = utils::isSymlink(linkPath);
    ASSERT_TRUE(isSymlinkResult.has_value());
    EXPECT_TRUE(isSymlinkResult.value());
    EXPECT_TRUE(fs::exists(linkPath));
    
    // 3. Read the symlink
    auto readResult = utils::readSymlink(linkPath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), targetPath);

    // 4. Check isSymlink on non-symlinks
    auto isNotSymlinkResult1 = utils::isSymlink(targetPath); // It's a regular file
    ASSERT_TRUE(isNotSymlinkResult1.has_value());
    EXPECT_FALSE(isNotSymlinkResult1.value());
    
    // is_symlink on a non-existent path returns false and ec is not set
    // according to the standard, so our wrapper should return a valid false.
    auto isNotSymlinkResult2 = utils::isSymlink(testDir / "nonexistent"); 
    // is_symlink on a non-existent path will fail, and our wrapper propagates the error.
    ASSERT_FALSE(isNotSymlinkResult2.has_value());


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
    auto isDanglingSymlink = utils::isSymlink(danglingLink);
    ASSERT_TRUE(isDanglingSymlink.has_value());
    EXPECT_TRUE(isDanglingSymlink.value());
    EXPECT_FALSE(fs::exists(danglingLink)); // Target doesn't exist
}

TEST_F(UtilsNewApiTest, GetAbsolutePath) {
    // 1. Simple relative path
    // Need to change current path to test this reliably
    fs::current_path(testDir);

    auto relPath = fs::path("some_file.txt");
    auto result1 = utils::getAbsolutePath(relPath);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), fs::absolute(relPath));

    // 2. Path with dots
    fs::create_directory("subdir");
    auto pathWithDots = fs::path("subdir/../some_file.txt");
    auto result2 = utils::getAbsolutePath(pathWithDots);
    ASSERT_TRUE(result2.has_value());
    // std::filesystem::absolute does not normalize, so we expect the .. to remain.
    EXPECT_EQ(result2.value(), fs::absolute(pathWithDots));

    // 3. Already absolute path
    auto absPath = testDir / "another_file.txt";
    auto result3 = utils::getAbsolutePath(absPath);
    ASSERT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value(), absPath);
    
    // 4. Error case. This is hard to trigger.
    // std::filesystem::absolute only fails if current_path() fails.
    // A path with invalid chars is not guaranteed to fail.
    // We will assume this is tested by other error propagation tests.
}

TEST_F(UtilsNewApiTest, PathComponents) {
    // 1. Normal path
    fs::path p1 = "/foo/bar/baz.txt";
    EXPECT_EQ(utils::getFileName(p1), "baz.txt");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p1), "baz");
    EXPECT_EQ(utils::getFileExtension(p1), ".txt");
    EXPECT_EQ(utils::getParentPath(p1), "/foo/bar");

    // 2. Path with multiple dots
    fs::path p2 = "/foo/bar/archive.tar.gz";
    EXPECT_EQ(utils::getFileName(p2), "archive.tar.gz");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p2), "archive.tar");
    EXPECT_EQ(utils::getFileExtension(p2), ".gz");
    EXPECT_EQ(utils::getParentPath(p2), "/foo/bar");

    // 3. Path with no extension
    fs::path p3 = "/foo/bar/baz";
    EXPECT_EQ(utils::getFileName(p3), "baz");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p3), "baz");
    EXPECT_EQ(utils::getFileExtension(p3), "");
    EXPECT_EQ(utils::getParentPath(p3), "/foo/bar");
    
    // 4. Path with just a filename
    fs::path p4 = "test.c";
    EXPECT_EQ(utils::getFileName(p4), "test.c");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p4), "test");
    EXPECT_EQ(utils::getFileExtension(p4), ".c");
    EXPECT_EQ(utils::getParentPath(p4), "");

    // 5. Root directory
    fs::path p5 = "/";
    EXPECT_EQ(utils::getFileName(p5), "");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p5), "");
    EXPECT_EQ(utils::getFileExtension(p5), "");
    EXPECT_EQ(utils::getParentPath(p5), "/");

    // 6. Dot paths
    fs::path p6 = ".";
    EXPECT_EQ(utils::getFileName(p6), ".");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p6), ".");
    EXPECT_EQ(utils::getFileExtension(p6), "");
    EXPECT_EQ(utils::getParentPath(p6), "");
    
    fs::path p7 = "..";
    EXPECT_EQ(utils::getFileName(p7), "..");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p7), "..");
    EXPECT_EQ(utils::getFileExtension(p7), "");
    EXPECT_EQ(utils::getParentPath(p7), "");

    // 7. Empty path
    fs::path p8 = "";
    EXPECT_EQ(utils::getFileName(p8), "");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p8), "");
    EXPECT_EQ(utils::getFileExtension(p8), "");
    EXPECT_EQ(utils::getParentPath(p8), "");
}

TEST_F(UtilsNewApiTest, JoinPaths) {
    // 1. Empty vector
    EXPECT_EQ(utils::joinPaths({}), "");

    // 2. Vector with one path
    EXPECT_EQ(utils::joinPaths({"a"}), "a");

    // 3. Joining relative paths
    fs::path expected1 = fs::path("a") / "b" / "c";
    EXPECT_EQ(utils::joinPaths({"a", "b", "c"}), expected1);

    // 4. Joining with an absolute path in the middle
    fs::path p_abs = fs::absolute(testDir);
    fs::path expected2 = p_abs / "bin";
    EXPECT_EQ(utils::joinPaths({"a", "b", p_abs, "bin"}), expected2);

    // 5. Joining with an absolute path at the start
    fs::path expected3 = p_abs / "share" / "doc";
    EXPECT_EQ(utils::joinPaths({p_abs, "share", "doc"}), expected3);

    // 6. Joining empty strings
    fs::path expected4 = fs::path("a") / "c";
    EXPECT_EQ(utils::joinPaths({"a", "", "c"}), expected4);
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
    
    // Verify actual write capability by attempting to append to the file.
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
    auto stopResult = utils::traverseDirectory(testDir, [&](const fs::directory_entry&) {
        count++;
        return utils::TraversalControl::stop;
    }, opts);
    ASSERT_FALSE(stopResult.has_value());
    EXPECT_EQ(stopResult.error(), utils::make_error_code(utils::UtilsError::traversalStopped));
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