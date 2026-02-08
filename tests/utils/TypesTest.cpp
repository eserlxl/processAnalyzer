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

// Test fixture for UtilsError tests
class TypesTest : public ::testing::Test {};

// 1. `make_error_code` Correctness
TEST_F(TypesTest, MakeErrorCodeCorrectness) {
    // Test a few enum members to verify `make_error_code`
    const utils::UtilsError err = utils::UtilsError::fileNotFound;
    const std::error_code ec = utils::make_error_code(err);

    // Check that the value corresponds to the enum's integer value
    EXPECT_EQ(ec.value(), static_cast<int>(err));

    // Check that the category is the correct custom category
    EXPECT_EQ(&ec.category(), &utils::utilsCategory());
    EXPECT_STREQ(ec.category().name(), "UtilsError");
}

// 2. Message Correctness
TEST_F(TypesTest, MessageCorrectness) {
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
        {utils::UtilsError::pathNotAbsolute, "Path is not absolute"},
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
        std::error_code ec = utils::make_error_code(err);
        EXPECT_EQ(ec.message(), msg) << "Message for error " << ec.value() << " is incorrect.";
    }
}

// 3. Default/Unknown Error Case
TEST_F(TypesTest, UnknownErrorMessage) {
    // Create an error code with a value that doesn't exist in the enum
    const int unknownErrorCodeValue = 999;
    const std::error_code ec(unknownErrorCodeValue, utils::utilsCategory());

    // Verify that the message is "Unknown error"
    EXPECT_EQ(ec.message(), "Unknown error");
}

// 4. `std::is_error_code_enum` Integration (Implicit Conversion)
TEST_F(TypesTest, ImplicitConversion) {
    // Thanks to the `std::is_error_code_enum` specialization, this conversion should work
    std::error_code ec = utils::UtilsError::permissionDenied;

    EXPECT_EQ(ec.value(), static_cast<int>(utils::UtilsError::permissionDenied));
    EXPECT_EQ(&ec.category(), &utils::utilsCategory());
    EXPECT_EQ(ec.message(), "Permission denied");
}

// 5. Success Condition (`UtilsError::none`)
TEST_F(TypesTest, SuccessCondition) {
    std::error_code ec = utils::make_error_code(utils::UtilsError::none);

    // An error code with value 0 should evaluate to false (no error)
    EXPECT_FALSE(ec);
    EXPECT_TRUE(!ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(ec.message(), "Success");

    std::error_code ecFail = utils::make_error_code(utils::UtilsError::ioError);
    // Any non-zero error code should evaluate to true (an error occurred)
    EXPECT_TRUE(ecFail);
    EXPECT_FALSE(!ecFail);
    EXPECT_NE(ecFail.value(), 0);
}
