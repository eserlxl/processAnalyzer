// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/Types.h"
#include <string>
#include <system_error>
#include <map>

// 1. `make_error_code` Correctness
TEST(TypesTest, MakeErrorCodeCorrectness) {
    // Test a few enum members to verify implicit conversion
    const utils::UtilsError err = utils::UtilsError::fileNotFound;
    const std::error_code ec = err; // Implicit conversion

    // Check that the value corresponds to the enum's integer value
    EXPECT_EQ(ec.value(), static_cast<int>(err));

    // Check that the category is the correct custom category
    EXPECT_EQ(&ec.category(), &utils::utilsErrorCategory());
    EXPECT_STREQ(ec.category().name(), "UtilsError");
}

// 2. Message Correctness
TEST(TypesTest, MessageCorrectness) {
    // A map of error enums to their expected message strings
    const std::map<utils::UtilsError, std::string> errorMessages = {
        {utils::UtilsError::none, "No error"},
        {utils::UtilsError::fileNotFound, "File or directory not found"},
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
        {utils::UtilsError::noSpaceOnDevice, "No space left on device"},
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
        std::error_code ec = err; // Implicit conversion
        EXPECT_EQ(ec.message(), msg) << "Message for error " << ec.value() << " is incorrect.";
    }
}

// 3. Default/Unknown Error Case
TEST(TypesTest, UnknownErrorMessage) {
    // Create an error code with a value that doesn't exist in the enum
    const int unknownErrorCodeValue = 999;
    const std::error_code ec(unknownErrorCodeValue, utils::utilsErrorCategory());

    // Verify that the message is "Unknown error"
    EXPECT_EQ(ec.message(), "Unknown UtilsError");
}

// 4. `std::is_error_code_enum` Integration (Implicit Conversion)
TEST(TypesTest, ImplicitConversion) {
    // Thanks to the `std::is_error_code_enum` specialization, this conversion should work
    std::error_code ec = utils::UtilsError::permissionDenied;

    EXPECT_EQ(ec.value(), static_cast<int>(utils::UtilsError::permissionDenied));
    EXPECT_EQ(&ec.category(), &utils::utilsErrorCategory());
    EXPECT_EQ(ec.message(), "Permission denied");
}

// 5. Success Condition (`UtilsError::none`)
TEST(TypesTest, SuccessCondition) {
    std::error_code ec = utils::UtilsError::none; // Implicit conversion

    // An error code with value 0 should evaluate to false (no error)
    EXPECT_FALSE(ec);
    EXPECT_TRUE(!ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(ec.message(), "No error");

    std::error_code ecFail = utils::UtilsError::ioError; // Implicit conversion
    // Any non-zero error code should evaluate to true (an error occurred)
    EXPECT_TRUE(ecFail);
    EXPECT_FALSE(!ecFail);
    EXPECT_NE(ecFail.value(), 0);
}

// New tests for std::error_condition mapping
TEST(TypesTest, ErrorConditionMappings) {
    const std::map<utils::UtilsError, std::errc> mappings = {
        {utils::UtilsError::fileNotFound, std::errc::no_such_file_or_directory},
        {utils::UtilsError::permissionDenied, std::errc::permission_denied},
        {utils::UtilsError::permissionDeniedCwd, std::errc::permission_denied},
        {utils::UtilsError::ioError, std::errc::io_error},
        {utils::UtilsError::invalidArgument, std::errc::invalid_argument},
        {utils::UtilsError::pathError, std::errc::invalid_argument},
        {utils::UtilsError::fileAlreadyExists, std::errc::file_exists},
        {utils::UtilsError::directoryNotEmpty, std::errc::directory_not_empty},
        {utils::UtilsError::notADirectory, std::errc::is_a_directory},
        {utils::UtilsError::isADirectory, std::errc::is_a_directory},
        {utils::UtilsError::diskFull, std::errc::no_space_on_device},
        {utils::UtilsError::noSpaceOnDevice, std::errc::no_space_on_device},
        {utils::UtilsError::commandNotFound, std::errc::no_such_process},
        {utils::UtilsError::commandExecutionError, std::errc::operation_not_permitted},
        {utils::UtilsError::commandFailed, std::errc::operation_not_permitted},
        {utils::UtilsError::processSpawnFailure, std::errc::operation_not_permitted},
        {utils::UtilsError::unsupportedOperation, std::errc::operation_not_supported}
    };

    for (const auto& [err, expected] : mappings) {
        std::error_code ec = err;
        EXPECT_TRUE(ec == std::errc(expected)) << "Error " << ec.value() << " (" << ec.message() << ") did not map to expected condition " << static_cast<int>(expected);
    }
}

TEST(TypesTest, ErrorConditionMapping_negativeMatch) {
    std::error_code ec = utils::UtilsError::fileNotFound;
    EXPECT_FALSE(ec == std::errc::permission_denied);
}
