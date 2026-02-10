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
}

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

class PathCoreTest : public TempDirTest {};

TEST_F(PathCoreTest, CanonicalPath) {
    auto fileAPath = testDir / "fileA.txt";
    std::ofstream(fileAPath) << "content";

    auto result1 = utils::canonicalPath(fileAPath);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), fs::canonical(fileAPath));

    auto pathWithDots = testDir / ".." / testDir.filename() / "fileA.txt";
    auto result2 = utils::canonicalPath(pathWithDots);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), fs::canonical(fileAPath));

    if (canCreateSymlinks()) {
        auto symlinkPath = testDir / "link_to_A";
        fs::create_symlink(fileAPath, symlinkPath);
        auto result3 = utils::canonicalPath(symlinkPath);
        ASSERT_TRUE(result3.has_value());
        EXPECT_EQ(result3.value(), fs::canonical(fileAPath));
    }

    auto nonExistentPath = testDir / "nonexistent.txt";
    auto result4 = utils::canonicalPath(nonExistentPath);
    ASSERT_FALSE(result4.has_value());
}

TEST_F(PathCoreTest, MakeRelative) {
    auto base = testDir / "a" / "b";
    auto path = testDir / "a" / "b" / "c" / "file.txt";
    fs::create_directories(base);

    auto result1 = utils::makeRelative(path, base);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), fs::path("c") / "file.txt");

    auto otherPath = testDir / "x" / "y";
    auto result2 = utils::makeRelative(otherPath, base);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), fs::path("..") / ".." / "x" / "y");

    auto result3 = utils::makeRelative(base, base);
    ASSERT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value(), ".");

    auto path4 = testDir / "a" / "d";
    auto result4 = utils::makeRelative(path4, base);
    ASSERT_TRUE(result4.has_value());
    EXPECT_EQ(result4.value(), fs::path("..") / "d");
}

TEST_F(PathCoreTest, PathsEquivalent) {
    auto fileAPath = testDir / "fileA.txt";
    std::ofstream(fileAPath) << "content";
    auto fileBPath = testDir / "fileB.txt";
    std::ofstream(fileBPath) << "content";

    auto result1 = utils::pathsEquivalent(fileAPath, fileAPath);
    ASSERT_TRUE(result1.has_value());
    EXPECT_TRUE(result1.value());

    auto pathWithDots = testDir / ".." / testDir.filename() / "fileA.txt";
    auto result2 = utils::pathsEquivalent(fileAPath, pathWithDots);
    ASSERT_TRUE(result2.has_value());
    EXPECT_TRUE(result2.value());

    if (canCreateSymlinks()) {
        auto symlinkPath = testDir / "link_to_A";
        fs::create_symlink(fileAPath, symlinkPath);
        auto result3 = utils::pathsEquivalent(fileAPath, symlinkPath);
        ASSERT_TRUE(result3.has_value());
        EXPECT_TRUE(result3.value());
    }

    auto result4 = utils::pathsEquivalent(fileAPath, fileBPath);
    ASSERT_TRUE(result4.has_value());
    EXPECT_FALSE(result4.value());

    auto nonExistentPath = testDir / "nonexistent.txt";
    EXPECT_FALSE(utils::pathsEquivalent(fileAPath, nonExistentPath).has_value());
    EXPECT_FALSE(utils::pathsEquivalent(nonExistentPath, fileAPath).has_value());
    EXPECT_FALSE(utils::pathsEquivalent(nonExistentPath, testDir / "another_nonexistent.txt").has_value());
}

TEST_F(PathCoreTest, GetAbsolutePath) {
    fs::current_path(testDir);

    auto relPath = fs::path("some_file.txt");
    auto result1 = utils::getAbsolutePath(relPath);
    ASSERT_TRUE(result1.has_value());
    EXPECT_EQ(result1.value(), fs::absolute(relPath));

    fs::create_directory("subdir");
    auto pathWithDots = fs::path("subdir/../some_file.txt");
    auto result2 = utils::getAbsolutePath(pathWithDots);
    ASSERT_TRUE(result2.has_value());
    EXPECT_EQ(result2.value(), fs::absolute(pathWithDots));

    auto absPath = testDir / "another_file.txt";
    auto result3 = utils::getAbsolutePath(absPath);
    ASSERT_TRUE(result3.has_value());
    EXPECT_EQ(result3.value(), absPath);
}

TEST_F(PathCoreTest, PathComponents) {
    fs::path p1 = "/foo/bar/baz.txt";
    EXPECT_EQ(utils::getFileName(p1), "baz.txt");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p1), "baz");
    EXPECT_EQ(utils::getFileExtension(p1), ".txt");
    EXPECT_EQ(utils::getParentPath(p1), "/foo/bar");

    fs::path p2 = "/foo/bar/archive.tar.gz";
    EXPECT_EQ(utils::getFileName(p2), "archive.tar.gz");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p2), "archive.tar");
    EXPECT_EQ(utils::getFileExtension(p2), ".gz");
    EXPECT_EQ(utils::getParentPath(p2), "/foo/bar");

    fs::path p3 = "/foo/bar/baz";
    EXPECT_EQ(utils::getFileName(p3), "baz");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p3), "baz");
    EXPECT_EQ(utils::getFileExtension(p3), "");
    EXPECT_EQ(utils::getParentPath(p3), "/foo/bar");
    
    fs::path p4 = "test.c";
    EXPECT_EQ(utils::getFileName(p4), "test.c");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p4), "test");
    EXPECT_EQ(utils::getFileExtension(p4), ".c");
    EXPECT_EQ(utils::getParentPath(p4), "");

    fs::path p5 = "/";
    EXPECT_EQ(utils::getFileName(p5), "");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p5), "");
    EXPECT_EQ(utils::getFileExtension(p5), "");
    EXPECT_EQ(utils::getParentPath(p5), "/");

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

    fs::path p8 = "";
    EXPECT_EQ(utils::getFileName(p8), "");
    EXPECT_EQ(utils::getFileNameWithoutExtension(p8), "");
    EXPECT_EQ(utils::getFileExtension(p8), "");
    EXPECT_EQ(utils::getParentPath(p8), "");
}

TEST_F(PathCoreTest, JoinPaths) {
    EXPECT_EQ(utils::joinPaths({}), "");

    EXPECT_EQ(utils::joinPaths({"a"}), "a");

    fs::path expected1 = fs::path("a") / "b" / "c";
    EXPECT_EQ(utils::joinPaths({"a", "b", "c"}), expected1);

    fs::path p_abs = fs::absolute(testDir);
    fs::path expected2 = p_abs / "bin";
    EXPECT_EQ(utils::joinPaths({"a", "b", p_abs, "bin"}), expected2);

    fs::path expected3 = p_abs / "share" / "doc";
    EXPECT_EQ(utils::joinPaths({p_abs, "share", "doc"}), expected3);

    fs::path expected4 = fs::path("a") / "c";
    EXPECT_EQ(utils::joinPaths({"a", "", "c"}), expected4);
}
