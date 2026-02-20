// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2024 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/filesystem.h"
#include "utils/file.h" // For readTextFile
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <ranges> // Required for std::ranges::find

namespace fs = std::filesystem;

namespace {
    constexpr int cleanupRetryCount = 3;
    constexpr int cleanupRetryDelayMs = 50;

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

class FilesystemOperationsTest : public TempDirTest {};

TEST_F(FilesystemOperationsTest, CreateDirectories) {
    fs::path newDir = testDir / "a" / "b" / "c";
    auto result = utils::createDirectories(newDir);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(fs::is_directory(newDir));
}

TEST_F(FilesystemOperationsTest, CreateDirectoriesExisting) {
    fs::path newDir = testDir / "a" / "b" / "c";
    ASSERT_TRUE(utils::createDirectories(newDir).has_value());
    auto result = utils::createDirectories(newDir); // Try to create it again
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(fs::is_directory(newDir));
}

TEST_F(FilesystemOperationsTest, CreateDirectoriesErrorHandling) {
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
    EXPECT_EQ(result.error(), std::errc::not_a_directory); 

    fs::remove(fileAsIntermediateDir);
}

TEST_F(FilesystemOperationsTest, RemoveFile) {
    fs::path file = testDir / "file.txt";
    {
        std::ofstream ofs(file);
        ofs << "test";
    }
    ASSERT_TRUE(fs::exists(file));
    auto result = utils::remove(file);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(fs::exists(file));
}

TEST_F(FilesystemOperationsTest, RemoveDirectoryRecursively) {
    fs::path dir = testDir / "a" / "b";
    fs::create_directories(dir);
    fs::path file = dir / "file.txt";
    {
        std::ofstream ofs(file);
        ofs << "test";
    }
    ASSERT_TRUE(fs::exists(testDir / "a"));
    auto result = utils::remove(testDir / "a", true);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(fs::exists(testDir / "a"));
}

TEST_F(FilesystemOperationsTest, RemoveErrorHandling) {
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

TEST_F(FilesystemOperationsTest, ListDirectory) {
    fs::path file1 = testDir / "file1.txt";
    fs::path file2 = testDir / "file2.txt";
    fs::path dir = testDir / "sub";
    fs::create_directory(dir);
    {
        std::ofstream ofs(file1);
        ofs << "1";
    }
    {
        std::ofstream ofs(file2);
        ofs << "2";
    }

    auto result = utils::listDirectory(testDir);
    ASSERT_TRUE(result.has_value());
    auto entries = result.value();
    ASSERT_EQ(entries.size(), 3);
    EXPECT_NE(std::ranges::find(entries, file1), entries.end());
    EXPECT_NE(std::ranges::find(entries, file2), entries.end());
    EXPECT_NE(std::ranges::find(entries, dir), entries.end());
}

TEST_F(FilesystemOperationsTest, ListDirectoryErrorHandling) {
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

    fs::permissions(inaccessibleDir, fs::perms::all);
}

TEST_F(FilesystemOperationsTest, CopyFile) {
    auto src = testDir / "src.txt";
    auto dst = testDir / "dst.txt";
    std::string content = "copy me";
    std::ofstream(src) << content;

    auto copyResult = utils::copyFile(src, dst);
    ASSERT_TRUE(copyResult.has_value());
    ASSERT_TRUE(fs::exists(dst));
    auto readRes = utils::readTextFile(dst);
    ASSERT_TRUE(readRes);
    EXPECT_EQ(readRes.value(), content);
}

TEST_F(FilesystemOperationsTest, CopyFileParentDirectoryCreation) {
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

    fs::permissions(restrictedParent, fs::perms::all);
}

TEST_F(FilesystemOperationsTest, CopyFileRejectsDirectorySource) {
    auto sourceDir = testDir / "source_dir";
    fs::create_directories(sourceDir / "child");
    std::ofstream(sourceDir / "child" / "file.txt") << "data";

    auto destinationFile = testDir / "copied.txt";
    auto result = utils::copyFile(sourceDir, destinationFile);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::invalidArgument));
    EXPECT_FALSE(fs::exists(destinationFile));
}

TEST_F(FilesystemOperationsTest, MoveFile) {
    fs::path source = testDir / "source_mv.txt";
    fs::path dest = testDir / "dest_mv.txt";
    {
        std::ofstream ofs(source);
        ofs << "test";
    }

    auto result = utils::moveFile(source, dest);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(fs::exists(source));
    EXPECT_TRUE(fs::exists(dest));
}

TEST_F(FilesystemOperationsTest, MoveFileCrossFilesystemFallback) {
    fs::path source = testDir / "source_mv_fs.txt";
    fs::path dest = testDir / "sub_mv_fs" / "dest_mv_fs.txt";
    {
        std::ofstream ofs(source);
        ofs << "test";
    }
    
    auto result = utils::moveFile(source, dest);
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(fs::exists(source));
    EXPECT_TRUE(fs::exists(dest));
}

TEST_F(FilesystemOperationsTest, TraverseDirectory) {
    auto root = testDir / "traverse";
    ASSERT_TRUE(utils::createDirectories(root / "dir1" / "subdir1"));
    ASSERT_TRUE(utils::createDirectories(root / "dir2"));
    std::ofstream(root / "root_file.txt") << "r";
    std::ofstream(root / "dir1" / "d1_file.txt") << "d1";
    std::ofstream(root / "dir1" / "subdir1" / "sd1_file.txt") << "sd1";
    std::ofstream(root / "dir2" / "d2_file.txt") << "d2";

    std::vector<fs::path> visited;
    utils::TraversalOptions opts;
    opts.recursive = true;
    
    EXPECT_TRUE(utils::traverseDirectory(root, [&](const fs::directory_entry& entry) {
        visited.push_back(entry.path());
        return utils::TraversalControl::Continue;
    }, opts).has_value());
    
    EXPECT_GE(visited.size(), 6);
}

TEST_F(FilesystemOperationsTest, TraverseDirectoryStop) {
    fs::path dirA = testDir / "a_stop";
    fs::path file1 = testDir / "file1_stop.txt";
    fs::create_directory(dirA);
    std::ofstream(file1) << "1";

    std::vector<fs::path> foundPaths;
    auto callback = [&](const fs::directory_entry& entry) {
        foundPaths.push_back(entry.path());
        return utils::TraversalControl::stop;
    };

    utils::TraversalOptions options;
    auto result = utils::traverseDirectory(testDir, callback, options);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::traversalStopped));
    EXPECT_EQ(foundPaths.size(), 1);
}

TEST_F(FilesystemOperationsTest, TraverseDirectorySkipDir) {
    auto root = testDir / "traverse_skip";
    ASSERT_TRUE(utils::createDirectories(root / "dir1" / "subdir1"));
    ASSERT_TRUE(utils::createDirectories(root / "dir2"));
    std::ofstream(root / "dir1" / "file_in_dir1.txt") << "f";

    std::vector<fs::path> visited;
    utils::TraversalOptions opts;
    opts.recursive = true;
    
    EXPECT_TRUE(utils::traverseDirectory(root, [&](const fs::directory_entry& entry) {
        visited.push_back(entry.path());
        if (entry.path().filename() == "dir1") {
            return utils::TraversalControl::skipDir;
        }
        return utils::TraversalControl::Continue;
    }, opts).has_value());
    
    bool visitedDir1 = false;
    bool visitedSubdir1 = false;
    bool visitedFileInDir1 = false;

    for (const auto& p : visited) {
        if (p.filename() == "dir1") visitedDir1 = true;
        if (p.filename() == "subdir1") visitedSubdir1 = true;
        if (p.filename() == "file_in_dir1.txt") visitedFileInDir1 = true;
    }
    EXPECT_TRUE(visitedDir1);
    EXPECT_FALSE(visitedSubdir1);
    EXPECT_FALSE(visitedFileInDir1);
}

TEST_F(FilesystemOperationsTest, SymlinkManagement) {
    if (!canCreateSymlinks()) {
        GTEST_SKIP() << "Symlink creation not supported (insufficient privileges or filesystem support).";
    }

    auto targetPath = testDir / "target.txt";
    std::ofstream(targetPath) << "symlink target";

    auto linkPath = testDir / "link.txt";

    auto createResult = utils::createSymlink(targetPath, linkPath);
    ASSERT_TRUE(createResult.has_value());
    
    auto readResult = utils::readSymlink(linkPath);
    ASSERT_TRUE(readResult.has_value());
    EXPECT_EQ(readResult.value(), targetPath);

    auto createAgainResult = utils::createSymlink(targetPath, linkPath);
    EXPECT_FALSE(createAgainResult.has_value());

    auto readNonLinkResult = utils::readSymlink(targetPath);
    EXPECT_FALSE(readNonLinkResult.has_value());

    auto danglingLink = testDir / "dangling";
    std::error_code ec;
    fs::create_symlink(testDir / "nonexistent_target", danglingLink, ec);
    ASSERT_FALSE(ec); // Creation should succeed even if target doesn't exist
    auto readDanglingResult = utils::readSymlink(danglingLink);
    ASSERT_TRUE(readDanglingResult.has_value());
    EXPECT_EQ(readDanglingResult.value(), testDir / "nonexistent_target");
}
