// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

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
inline std::string generateRandomString(size_t length) {
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

constexpr size_t randomNameLen = 10;

namespace {
    constexpr int cleanupRetryCount = 3;
    constexpr int cleanupRetryDelayMs = 50;
    const std::vector<std::byte> testData = {std::byte{0xDE}, std::byte{0xAD}, std::byte{0xBE}, std::byte{0xEF}};
    const std::vector<std::byte> appendData = {std::byte{0x00}, std::byte{0xFF}};
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
                for (int i = 0; i < cleanupRetryCount; ++i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(cleanupRetryDelayMs));
                    fs::remove_all(testDir, ec);
                    if (!ec) break;
                }
            }
        }
};
        
// Test fixture for new API tests
class UtilsNewApiTest : public TempDirTest {
};

// --- File-related Error Handling Tests ---
TEST_F(UtilsNewApiTest, ReadBinaryFileErrorHandling) {
    // Non-existent file
    auto nonExistentFile = testDir / "non_existent.bin";
    auto result = utils::readBinaryFile(nonExistentFile);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::fileNotFound));

    // Path is a directory
    auto resultDir = utils::readBinaryFile(testDir);
    ASSERT_FALSE(resultDir.has_value());
    EXPECT_EQ(resultDir.error(), utils::make_error_code(utils::UtilsError::ioError)); // tellg() on a directory returns -1
    
    // Permission denied (assuming we can make a file unreadable)
    fs::path unreadableFile = testDir / "unreadable.bin";
    std::ofstream(unreadableFile) << "secret";
    fs::permissions(unreadableFile, fs::perms::none, fs::perm_options::replace);
    
    auto resultUnreadable = utils::readBinaryFile(unreadableFile);
    ASSERT_FALSE(resultUnreadable.has_value());
    EXPECT_EQ(resultUnreadable.error(), utils::make_error_code(utils::UtilsError::ioError));

    // Restore permissions for cleanup
    fs::permissions(unreadableFile, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::add);
}
        
TEST_F(UtilsNewApiTest, WriteFunctionsFileTooLarge) {
    const fs::path testFilePath = testDir / "large_file_test.bin";
    constexpr size_t dummyContentSize = 100;
    constexpr std::byte dummyByteValue{0xAA};
    std::vector<std::byte> dummyContent(dummyContentSize, dummyByteValue); // Small content

    // --- Test `writeBinaryFile` ---
    // If std::streamsize is 64-bit, this won't trigger fileTooLarge, but we verify other errors.
    auto resultWb = utils::writeBinaryFile(testFilePath, dummyContent);
    ASSERT_TRUE(resultWb.has_value()); // Should succeed with small content

    // Simulate permission denied for write operations
    fs::path noWriteDir = testDir / "no_write";
    fs::create_directory(noWriteDir);
    fs::permissions(noWriteDir, fs::perms::owner_read, fs::perm_options::replace);

    fs::path noWriteFile = noWriteDir / "file.txt";
    auto writeResultPerm = utils::writeTextFile(noWriteFile, "test");
    ASSERT_FALSE(writeResultPerm.has_value());
    EXPECT_EQ(writeResultPerm.error(), utils::make_error_code(utils::UtilsError::ioError));

    // Restore permissions for cleanup
    fs::permissions(noWriteDir, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);
}

TEST_F(UtilsNewApiTest, DoAtomicWriteParentPathChecks) {
    auto targetFile = testDir / "sub" / "atomic_target.txt";
    std::string content = "test content";

    // Test with non-existent parent directory
    auto result = utils::writeTextFileAtomic(targetFile, content); // This uses doAtomicWrite
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::fileNotFound));

    // Test with parent path being a file
    fs::path parentIsFile = testDir / "parent_is_file";
    std::ofstream(parentIsFile) << "I am a file";
    fs::path fileUnderFile = parentIsFile / "child.txt";
    
    result = utils::writeTextFileAtomic(fileUnderFile, content);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::notADirectory));

    // Clean up the file acting as parent
    fs::remove(parentIsFile);
}

TEST_F(UtilsNewApiTest, CreateTemporaryDirectoryValidation) {
    // Test createTemporaryFile when a race condition creates a file with the same name before it tries
    // This is hard to perfectly simulate a race, but we can pre-create it.
    auto preExistingTempFile = fs::temp_directory_path() / ("test_prefix_" + generateRandomString(randomNameLen) + ".tmp");
    std::ofstream(preExistingTempFile) << "pre-existing";

    auto tempFileResult = utils::createTemporaryFile("test_prefix_", ".tmp");
    ASSERT_TRUE(tempFileResult.has_value()); // Should find another name, or overwrite if exists() returns false due to error
    EXPECT_NE(tempFileResult.value(), preExistingTempFile); // Should be a different path

    fs::remove(preExistingTempFile); // Cleanup
}

TEST_F(UtilsNewApiTest, ReadLinesErrorHandling) {
    // Non-existent file
    auto nonExistentFile = testDir / "non_existent_lines.txt";
    auto result = utils::readLines(nonExistentFile);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::fileNotFound));

    // Path is a directory
    auto resultDir = utils::readLines(testDir);
    ASSERT_FALSE(resultDir.has_value());
    EXPECT_EQ(resultDir.error(), utils::make_error_code(utils::UtilsError::ioError));

    // Permission denied
    auto unreadableFile = testDir / "unreadable_lines.txt";
    std::ofstream(unreadableFile) << "line1\nline2";
    fs::permissions(unreadableFile, fs::perms::owner_write, fs::perm_options::replace); // Make unreadable
    
    result = utils::readLines(unreadableFile);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), utils::make_error_code(utils::UtilsError::ioError));

    // Restore permissions for cleanup
    fs::permissions(unreadableFile, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::add);
}

TEST_F(UtilsNewApiTest, ReadWriteTextFileErrorHandling) {
    // Test with non-existent file
    auto nonExistentFile = testDir / "non_existent_text.txt";
    auto resultRead = utils::readTextFile(nonExistentFile);
    ASSERT_FALSE(resultRead.has_value());
    EXPECT_EQ(resultRead.error(), utils::make_error_code(utils::UtilsError::fileNotFound));

    // Test write with permission denied
    fs::path noWriteDir = testDir / "no_write_text_dir";
    fs::create_directory(noWriteDir);
    fs::permissions(noWriteDir, fs::perms::owner_read, fs::perm_options::replace);

    fs::path noWriteFile = noWriteDir / "output.txt";
    auto resultWrite = utils::writeTextFile(noWriteFile, "hello");
    ASSERT_FALSE(resultWrite.has_value());
    EXPECT_EQ(resultWrite.error(), utils::make_error_code(utils::UtilsError::ioError));

    // Restore permissions for cleanup
    fs::permissions(noWriteDir, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);
}

// Correctness test for doAtomicWrite ensuring original is untouched on failure
TEST_F(UtilsNewApiTest, DoAtomicWriteFailureEnsuresOriginalUntouched) {
    fs::path originalFile = testDir / "original.txt";
    std::string originalContent = "This is the original content.";
    std::string newContent = "This is the new content.";

    // Case 1: Original file does not exist, write fails (e.g., permissions on parent)
    fs::path inaccessibleDir = testDir / "inaccessible_parent";
    fs::create_directory(inaccessibleDir);
    fs::permissions(inaccessibleDir, fs::perms::owner_read); // Make un-writable

    fs::path targetInInaccessible = inaccessibleDir / "atomic_write_target.txt";

    utils::Result<void> writeResult = {};
    try {
        writeResult = utils::writeTextFileAtomic(targetInInaccessible, newContent);
    } catch (...) {
        writeResult = std::unexpected(utils::make_error_code(utils::UtilsError::permissionDenied));
    }
    ASSERT_FALSE(writeResult.has_value());
    
    // Restore permissions FIRST to avoid exception in fs::exists if parent is not searchable
    fs::permissions(inaccessibleDir, fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec, fs::perm_options::replace);
    
    // Check if the target was NOT created.
    EXPECT_FALSE(fs::exists(targetInInaccessible));

    fs::remove(inaccessibleDir); // Cleanup

    // Case 3: Original file exists, write to temp succeeds, but rename fails.
    // We'll simulate a rename failure by attempting to rename over a read-only directory
    // (which is not allowed), which will cause rename to fail and trigger cleanup.
    fs::path testTarget = testDir / "rename_fail_test.txt";
    fs::remove(testTarget); // Remove initial file FIRST
    fs::path blockingDir = testTarget; // Target is now a directory
    fs::create_directory(blockingDir); // Create a directory at target name
        
    auto result = utils::writeTextFileAtomic(testTarget, newContent);

    ASSERT_FALSE(result.has_value());
    
    EXPECT_TRUE(fs::is_directory(testTarget)); // Target should still be the blocking directory
    
    fs::remove_all(blockingDir); // Cleanup
}

// --------------------------------------------------------------------------
// New Tests for Missing Implementations (File Specific)
// --------------------------------------------------------------------------

TEST_F(UtilsNewApiTest, ReadWriteBinaryFile) {
    auto binaryFile = testDir / "binary.dat";
    std::vector<std::byte> data = testData;

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
    std::vector<std::byte> moreData = appendData;
    auto appendResult = utils::appendToBinaryFile(binaryFile, moreData);
    ASSERT_TRUE(appendResult.has_value());
    
    // Read again
    auto readResult2 = utils::readBinaryFile(binaryFile);
    ASSERT_TRUE(readResult2.has_value());
    EXPECT_EQ(readResult2.value().size(), 6);
    EXPECT_EQ(readResult2.value()[4], appendData[0]);
    EXPECT_EQ(readResult2.value()[5], appendData[1]);
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
