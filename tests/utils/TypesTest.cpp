// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/Types.h"
#include <string>
#include <system_error>
#include <map>

// Externally declared function from Types.cpp for testing purposes
namespace utils {
const std::error_category& utilsCategory();
}

// 1. `make_error_code` Correctness
TEST(TypesTest, MakeErrorCodeCorrectness) {
    // Test a few enum members to verify implicit conversion
    const utils::UtilsError err = utils::UtilsError::fileNotFound;
    const std::error_code ec = err; // Implicit conversion

    // Check that the value corresponds to the enum's integer value
    EXPECT_EQ(ec.value(), static_cast<int>(err));

    // Check that the category is the correct custom category
    EXPECT_EQ(&ec.category(), &utils::utilsCategory());
    EXPECT_STREQ(ec.category().name(), "UtilsError");
}

// 2. Message Correctness
TEST(TypesTest, MessageCorrectness) {
    // A map of error enums to their expected message strings
    const std::map<utils::UtilsError, std::string> errorMessages = {
        {utils::UtilsError::none, "Success"},
        {utils::UtilsError::fileNotFound, "File not found"},
        {utils::UtilsError::permissionDenied, "Permission denied"},
        {utils::UtilsError::ioError, "I/O error"},
        {utils::UtilsError::invalidArgument, "Invalid argument"},
        {utils::UtilsError::unsupportedOperation, "Unsupported operation"},
        {utils::UtilsError::pathError, "Path error"},
        {utils::UtilsError::commandExecutionError, "Command execution error"},
        {utils::UtilsError::fileAlreadyExists, "File already exists"},
        {utils::UtilsError::directoryNotEmpty, "Directory not empty"},
        {utils::UtilsError::notADirectory, "Not a directory"},
        {utils::UtilsError::notAFile, "Not a file"},
        {utils::UtilsError::isADirectory, "Is a directory"},
        {utils::UtilsError::diskFull, "Disk full"},
        {utils::UtilsError::noSpaceOnDevice, "No space on device"},
        {utils::UtilsError::pathNotRelative, "Path is not relative"},
        {utilsError::pathNotAbsolute, "Path is not absolute"},
        {utils::UtilsError::basePathNotAncestor, "Base path is not an ancestor"},
        {utils::UtilsError::invalidPathFormat, "Invalid path format"},
        {utils::UtilsError::invalidBase64Input, "Invalid Base64 input"},
        {utils::UtilsError::invalidUrlEncoding, "Invalid URL encoding"},
        {utils::UtilsError::invalidUuidFormat, "Invalid UUID format"},
        {utils::UtilsError::envVarNotFound, "Environment variable not found"},
        {utils::UtilsError::commandNotFound, "Command not found"},
        {utils::UtilsError::commandFailed, "Command failed"},
        {utils::UtilsError::processSpawnFailure, "Process spawn failure"},
        {utils::UtilsError::permissionDeniedCwd, "Permission denied for changing CWD"},
        {utils::UtilsError::invalidTimeFormat, "Invalid time format"},
        {utils::UtilsError::timeParseError, "Time parsing error"},
    };

    for (const auto& [err, msg] : errorMessages) {
        std::error_code ec = err; // Implicit conversion
        EXPECT_EQ(ec.message(), msg) << "Message for error " << ec.value() << " is incorrect.";
    }
}

// 3. Default/Unknown Error Case
TEST(TypesTest, UnknownErrorMessage) {
    // Create an error code with a value that doesn't exist in the enum
    const int unknownErrorCodeValue = 999;
    const std::error_code ec(unknownErrorCodeValue, utils::utilsCategory());

    // Verify that the message is "Unknown error"
    EXPECT_EQ(ec.message(), "Unknown error");
}

// 4. `std::is_error_code_enum` Integration (Implicit Conversion)
TEST(TypesTest, ImplicitConversion) {
    // Thanks to the `std::is_error_code_enum` specialization, this conversion should work
    std::error_code ec = utils::UtilsError::permissionDenied;

    EXPECT_EQ(ec.value(), static_cast<int>(utils::UtilsError::permissionDenied));
    EXPECT_EQ(&ec.category(), &utils::utilsCategory());
    EXPECT_EQ(ec.message(), "Permission denied");
}

// 5. Success Condition (`UtilsError::none`)
TEST(TypesTest, SuccessCondition) {
    std::error_code ec = utils::UtilsError::none; // Implicit conversion

    // An error code with value 0 should evaluate to false (no error)
    EXPECT_FALSE(ec);
    EXPECT_TRUE(!ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(ec.message(), "Success");

    std::error_code ecFail = utils::UtilsError::ioError; // Implicit conversion
    // Any non-zero error code should evaluate to true (an error occurred)
    EXPECT_TRUE(ecFail);
    EXPECT_FALSE(!ecFail);
    EXPECT_NE(ecFail.value(), 0);
}

// New tests for std::error_condition mapping
TEST(TypesTest, ErrorConditionMapping_fileNotFound) {
    std::error_code ec = utils::UtilsError::fileNotFound;
    EXPECT_TRUE(ec == std::errc::no_such_file_or_directory);
    EXPECT_TRUE(ec.equivalent(std::errc::no_such_file_or_directory));
}

TEST(TypesTest, ErrorConditionMapping_permissionDenied) {
    std::error_code ec = utils::UtilsError::permissionDenied;
    EXPECT_TRUE(ec == std::errc::permission_denied);
    EXPECT_TRUE(ec.equivalent(std::errc::permission_denied));

    std::error_code ecCwd = utils::UtilsError::permissionDeniedCwd;
    EXPECT_TRUE(ecCwd == std::errc::permission_denied);
    EXPECT_TRUE(ecCwd.equivalent(std::errc::permission_denied));
}

TEST(TypesTest, ErrorConditionMapping_ioError) {
    std::error_code ec = utils::UtilsError::ioError;
    EXPECT_TRUE(ec == std::errc::io_error);
    EXPECT_TRUE(ec.equivalent(std::errc::io_error));
}

TEST(TypesTest, ErrorConditionMapping_invalidArgument) {
    std::error_code ec = utils::UtilsError::invalidArgument;
    EXPECT_TRUE(ec == std::errc::invalid_argument);
    EXPECT_TRUE(ec.equivalent(std::errc::invalid_argument));

    std::error_code ecPathError = utils::UtilsError::pathError;
    EXPECT_TRUE(ecPathError == std::errc::invalid_argument);
    EXPECT_TRUE(ecPathError.equivalent(std::errc::invalid_argument));
}

TEST(TypesTest, ErrorConditionMapping_fileAlreadyExists) {
    std::error_code ec = utils::UtilsError::fileAlreadyExists;
    EXPECT_TRUE(ec == std::errc::file_exists);
    EXPECT_TRUE(ec.equivalent(std::errc::file_exists));
}

TEST(TypesTest, ErrorConditionMapping_directoryNotEmpty) {
    std::error_code ec = utils::UtilsError::directoryNotEmpty;
    EXPECT_TRUE(ec == std::errc::directory_not_empty);
    EXPECT_TRUE(ec.equivalent(std::errc::directory_not_empty));
}

TEST(TypesTest, ErrorConditionMapping_notADirectory_isADirectory) {
    std::error_code ecNotADir = utils::UtilsError::notADirectory;
    EXPECT_TRUE(ecNotADir == std::errc::is_a_directory);
    EXPECT_TRUE(ecNotADir.equivalent(std::errc::is_a_directory));

    std::error_code ecIsADir = utils::UtilsError::isADirectory;
    EXPECT_TRUE(ecIsADir == std::errc::is_a_directory);
    EXPECT_TRUE(ecIsADir.equivalent(std::errc::is_a_directory));
}

TEST(TypesTest, ErrorConditionMapping_diskFull) {
    std::error_code ec = utils::UtilsError::diskFull;
    EXPECT_TRUE(ec == std::errc::no_space_on_device);
    EXPECT_TRUE(ec.equivalent(std::errc::no_space_on_device));

    std::error_code ecNoSpace = utils::UtilsError::noSpaceOnDevice;
    EXPECT_TRUE(ecNoSpace == std::errc::no_space_on_device);
    EXPECT_TRUE(ecNoSpace.equivalent(std::errc::no_space_on_device));
}

TEST(TypesTest, ErrorConditionMapping_commandNotFound) {
    std::error_code ec = utils::UtilsError::commandNotFound;
    EXPECT_TRUE(ec == std::errc::no_such_process);
    EXPECT_TRUE(ec.equivalent(std::errc::no_such_process));
}

TEST(TypesTest, ErrorConditionMapping_commandExecutionErrors) {
    std::error_code ecExec = utils::UtilsError::commandExecutionError;
    EXPECT_TRUE(ecExec == std::errc::operation_not_permitted);
    EXPECT_TRUE(ecExec.equivalent(std::errc::operation_not_permitted));

    std::error_code ecFailed = utils::UtilsError::commandFailed;
    EXPECT_TRUE(ecFailed == std::errc::operation_not_permitted);
    EXPECT_TRUE(ecFailed.equivalent(std::errc::operation_not_permitted));

    std::error_code ecSpawn = utils::UtilsError::processSpawnFailure;
    EXPECT_TRUE(ecSpawn == std::errc::operation_not_permitted);
    EXPECT_TRUE(ecSpawn.equivalent(std::errc::operation_not_permitted));
}

TEST(TypesTest, ErrorConditionMapping_unsupportedOperation) {
    std::error_code ec = utils::UtilsError::unsupportedOperation;
    EXPECT_TRUE(ec == std::errc::operation_not_supported);
    EXPECT_TRUE(ec.equivalent(std::errc::operation_not_supported));
}

TEST(TypesTest, ErrorConditionMapping_negativeMatch) {
    std::error_code ec = utils::UtilsError::fileNotFound;
    EXPECT_FALSE(ec == std::errc::permission_denied);
    EXPECT_FALSE(ec.equivalent(std::errc::permission_denied));
}
