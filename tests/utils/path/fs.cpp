// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/core.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

namespace fs = std::filesystem;

namespace {
    constexpr int cleanupRetryCount = 3;
    constexpr int cleanupRetryDelayMs = 50;


// Base fixture for tests requiring a temporary directory
class TempDirTest : public ::testing::Test {
protected:
    fs::path testDir;
    fs::path originalPath;

    void SetUp() override {
            const ::testing::TestInfo* const testInfo =
                ::testing::UnitTest::GetInstance()->current_test_info();
            testDir = fs::temp_directory_path() / (std::string(testInfo->test_suite_name()) + "_" + testInfo->name());
            
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
                for (int i = 0; i < cleanupRetryCount; ++i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cleanupRetryDelayMs));
                    fs::remove_all(testDir, ec);
                    if (!ec) break;
                }
            }
        }
        
        bool canCreateSymlinks() {
            std::error_code ec;
            auto target = testDir / "symlink_test_target";
            auto link = testDir / "symlink_test_link";
            
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
}

class PathFsTest : public TempDirTest {};

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

TEST_F(PathFsTest, TraverseDirectoryTraversalStopped) {
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
    EXPECT_EQ(count, 2); 
}

TEST_F(PathFsTest, ExistsIsFileIsDirectoryErrorPropagation) {
    fs::path inaccessibleDir = testDir / "inaccessible";
    fs::create_directory(inaccessibleDir);
    fs::path childFile = inaccessibleDir / "child.txt";
    std::ofstream(childFile) << "content";

    fs::permissions(inaccessibleDir, fs::perms::none);

    auto existsResult = utils::exists(childFile);
    ASSERT_FALSE(existsResult.has_value());
    EXPECT_TRUE(existsResult.error() == std::errc::permission_denied); 

    auto isFileResult = utils::isFile(childFile);
    ASSERT_FALSE(isFileResult.has_value());
    EXPECT_TRUE(isFileResult.error() == std::errc::permission_denied);

    auto isDirectoryResult = utils::isDirectory(childFile);
    ASSERT_FALSE(isDirectoryResult.has_value());
    EXPECT_TRUE(isDirectoryResult.error() == std::errc::permission_denied);

    fs::permissions(inaccessibleDir, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);
}

TEST_F(PathFsTest, CreateDirectoriesErrorHandling) {
    fs::path noWriteParent = testDir / "no_write_parent";
    fs::create_directory(noWriteParent);
    fs::permissions(noWriteParent, fs::perms::owner_read); 

    auto inaccessibleSubDir = noWriteParent / "new_dir";
    auto result = utils::createDirectories(inaccessibleSubDir);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::permissionDenied));

    fs::permissions(noWriteParent, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);

    fs::path fileAsIntermediateDir = testDir / "file_here";
    std::ofstream(fileAsIntermediateDir) << "content";

    auto pathToCreate = fileAsIntermediateDir / "sub_dir" / "another_sub";
    result = utils::createDirectories(pathToCreate);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), std::errc::invalid_argument); 

    fs::remove(fileAsIntermediateDir);
}

TEST_F(PathFsTest, RemoveErrorHandling) {
    auto nonExistent = testDir / "no_such_thing";
    auto result = utils::remove(nonExistent, false);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::fileNotFound));

    fs::path readOnlyDir = testDir / "read_only_dir";
    fs::create_directory(readOnlyDir);
    auto protectedFile = readOnlyDir / "protected.txt";
    std::ofstream(protectedFile) << "secret";
    
    fs::permissions(readOnlyDir, fs::perms::owner_read | fs::perms::owner_exec, fs::perm_options::replace); 
    
    result = utils::remove(protectedFile, false);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::permissionDenied));

    fs::permissions(readOnlyDir, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);
    fs::remove_all(readOnlyDir);
}

TEST_F(PathFsTest, ListDirectoryErrorHandling) {
    auto nonExistentDir = testDir / "no_such_dir";
    auto result = utils::listDirectory(nonExistentDir);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::fileNotFound));

    auto fileInsteadOfDir = testDir / "my_file.txt";
    std::ofstream(fileInsteadOfDir) << "content";
    
    result = utils::listDirectory(fileInsteadOfDir);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::ioError)); 

    fs::path inaccessibleDir = testDir / "inaccessible_for_list";
    fs::create_directory(inaccessibleDir);
    fs::permissions(inaccessibleDir, fs::perms::none);

    auto resultInacc = utils::listDirectory(inaccessibleDir);
    ASSERT_FALSE(resultInacc.has_value());
    EXPECT_TRUE(resultInacc.error() == std::errc::permission_denied);

    fs::permissions(inaccessibleDir, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);
}

TEST_F(PathFsTest, CopyFileParentDirectoryCreation) {
    auto sourceFile = testDir / "source.txt";
    std::ofstream(sourceFile) << "original content";

    auto destinationDir = testDir / "new_parent" / "sub_folder";
    auto destinationFile = destinationDir / "destination.txt";

    auto result = utils::copyFile(sourceFile, destinationFile);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(fs::exists(destinationFile));
    EXPECT_TRUE(fs::exists(destinationDir));
    
    auto readResult = utils::readTextFile(destinationFile);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), "original content");

    fs::path restrictedParent = testDir / "restricted_copy_target";
    fs::create_directory(restrictedParent);
    fs::permissions(restrictedParent, fs::perms::owner_read); 

    auto inaccessibleDestination = restrictedParent / "sub_dir" / "file.txt";
    result = utils::copyFile(sourceFile, inaccessibleDestination);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::permissionDenied));

    fs::permissions(restrictedParent, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);
}

TEST_F(PathFsTest, CopyFileRejectsDirectorySource) {
    auto sourceDir = testDir / "source_dir";
    fs::create_directories(sourceDir / "child");
    std::ofstream(sourceDir / "child" / "file.txt") << "data";

    auto destinationFile = testDir / "copied.txt";
    auto result = utils::copyFile(sourceDir, destinationFile);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
    EXPECT_FALSE(fs::exists(destinationFile));
}

TEST_F(PathFsTest, SymlinkManagement) {
    if (!canCreateSymlinks()) {
        GTEST_SKIP() << "Symlink creation not supported (insufficient privileges or filesystem support).";
    }

    auto targetPath = testDir / "target.txt";
    std::ofstream(targetPath) << "symlink target";

    auto linkPath = testDir / "link.txt";

    auto createResult = utils::createSymlink(targetPath, linkPath);
    ASSERT_TRUE(createResult.has_value());

    auto isSymlinkResult = utils::isSymlink(linkPath);
    ASSERT_TRUE(isSymlinkResult.has_value());
    EXPECT_TRUE(isSymlinkResult.value());
    EXPECT_TRUE(fs::exists(linkPath));
    
    auto readResult = utils::readSymlink(linkPath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), targetPath);

    auto isNotSymlinkResult1 = utils::isSymlink(targetPath); 
    ASSERT_TRUE(isNotSymlinkResult1.has_value());
    EXPECT_FALSE(isNotSymlinkResult1.value());
    
    auto isNotSymlinkResult2 = utils::isSymlink(testDir / "nonexistent"); 
    ASSERT_FALSE(isNotSymlinkResult2.has_value());


    auto createAgainResult = utils::createSymlink(targetPath, linkPath);
    EXPECT_FALSE(createAgainResult.has_value());

    auto readNonLinkResult = utils::readSymlink(targetPath);
    EXPECT_FALSE(readNonLinkResult.has_value());

    auto danglingLink = testDir / "dangling";
    fs::create_symlink(testDir / "nonexistent_target", danglingLink);
    auto isDanglingSymlink = utils::isSymlink(danglingLink);
    ASSERT_TRUE(isDanglingSymlink.has_value());
    EXPECT_TRUE(isDanglingSymlink.value());
    EXPECT_FALSE(fs::exists(danglingLink)); 
}

TEST_F(UtilsPermissionsTest, GetAndSetPermissions) {
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

TEST_F(UtilsPermissionsTest, AddAndRemovePermissions) {
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

TEST_F(UtilsPermissionsTest, ReadWriteExecutableChecks) {
    EXPECT_TRUE(utils::isReadable(testFile));
    EXPECT_TRUE(utils::isWritable(testFile));
    
    {
        std::ofstream os(testFile, std::ios::app);
        EXPECT_TRUE(os.good());
    }

    #ifndef _WIN32
    EXPECT_FALSE(utils::isExecutable(testFile));
    #endif

    ASSERT_TRUE(utils::setPermissions(testFile, fs::perms::owner_read).has_value());
    EXPECT_TRUE(utils::isReadable(testFile));
    EXPECT_FALSE(utils::isWritable(testFile));
    
    {
        std::ofstream os(testFile, std::ios::app);
        EXPECT_FALSE(os.good());
    }

    #ifndef _WIN32
    EXPECT_FALSE(utils::isExecutable(testFile));
    #endif
    
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

TEST_F(PathFsTest, TraverseDirectory) {
    auto sub1 = testDir / "sub1";
    auto sub2 = testDir / "sub2";
    auto sub3 = sub2 / "sub3";
    fs::create_directories(sub1);
    fs::create_directories(sub3);
    
    std::ofstream(testDir / "file1.txt") << "1";
    std::ofstream(sub1 / "file2.txt") << "2";
    std::ofstream(sub3 / "file3.txt") << "3";

    std::vector<fs::path> visited;
    utils::TraversalOptions opts;
    opts.recursive = true;
    
    EXPECT_TRUE(utils::traverseDirectory(testDir, [&](const fs::directory_entry& entry) {
        visited.push_back(entry.path());
        return utils::TraversalControl::Continue;
    }, opts).has_value());
    
    EXPECT_GE(visited.size(), 6);

    visited.clear();
    opts.recursive = false;
    EXPECT_TRUE(utils::traverseDirectory(testDir, [&](const fs::directory_entry& entry) {
        visited.push_back(entry.path());
        return utils::TraversalControl::Continue;
    }, opts).has_value());
    
    EXPECT_EQ(visited.size(), 3);

    int count = 0;
    auto stopResult = utils::traverseDirectory(testDir, [&](const fs::directory_entry&) {
        count++;
        return utils::TraversalControl::stop;
    }, opts);
    ASSERT_FALSE(stopResult.has_value());
    EXPECT_EQ(stopResult.error(), utils::make_error_code(utils::UtilsError::traversalStopped));
    EXPECT_EQ(count, 1);

    visited.clear();
    opts.recursive = true;
    EXPECT_TRUE(utils::traverseDirectory(testDir, [&](const fs::directory_entry& entry) {
        visited.push_back(entry.path());
        if (entry.is_directory() && entry.path().filename() == "sub1") {
            return utils::TraversalControl::skipDir;
        }
        return utils::TraversalControl::Continue;
    }, opts).has_value());
    
    bool visitedSub1 = false;
    bool visitedFile2 = false;
    for(const auto& p : visited) {
        if (p.filename() == "sub1") visitedSub1 = true;
        if (p.filename() == "file2.txt") visitedFile2 = true;
    }
    EXPECT_TRUE(visitedSub1); 
    EXPECT_FALSE(visitedFile2); 
}
