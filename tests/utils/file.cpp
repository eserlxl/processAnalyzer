// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/core.h"
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
}
        
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
}

TEST_F(UtilsNewApiTest, TemporaryFiles) {
    auto tempFile = utils::createTemporaryFile("test_prefix_", ".tmp");
    ASSERT_TRUE(tempFile.has_value());
    EXPECT_TRUE(fs::exists(tempFile.value()));
    EXPECT_TRUE(fs::is_regular_file(tempFile.value()));

    auto tempFile2 = utils::createTemporaryFile("test_prefix_", ".tmp");
    ASSERT_TRUE(tempFile2.has_value());
    EXPECT_NE(tempFile.value(), tempFile2.value());
    
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
    fs::remove(tempFile2.value());
    fs::remove_all(tempDir.value());
}
