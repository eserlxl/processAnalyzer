// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "gtest/gtest.h"
#include "utils/Types.h"
#include <string>
#include <system_error>
#include <map>
#include <vector>
#include <set>

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
        {utils::UtilsError::fileTooLarge, "File is too large to process"},
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
        {utils::UtilsError::traversalStopped, "Directory traversal stopped by callback"},
        {utils::UtilsError::tempDirectoryError, "Temporary directory error"},
        {utils::UtilsError::outOfRange, "Value out of range"},
        {utils::UtilsError::analyzerProcessNotFound, "Analyzer: Process not found"},
        {utils::UtilsError::analyzerParsingError, "Analyzer: Parsing error"},
        {utils::UtilsError::analyzerSystemError, "Analyzer: System error"},
        {utils::UtilsError::analyzerPermissionDenied, "Analyzer: Permission denied"},
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

// 4. Success Condition (`UtilsError::none`)
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

// 5. New tests for std::error_condition mapping
TEST(TypesTest, ErrorConditionMappings) {
    const std::map<utils::UtilsError, std::errc> mappings = {
        {utils::UtilsError::fileNotFound, std::errc::no_such_file_or_directory},
        {utils::UtilsError::permissionDenied, std::errc::permission_denied},
        {utils::UtilsError::permissionDeniedCwd, std::errc::permission_denied},
        {utils::UtilsError::analyzerPermissionDenied, std::errc::permission_denied},
        {utils::UtilsError::ioError, std::errc::io_error},
        {utils::UtilsError::invalidArgument, std::errc::invalid_argument},
        {utils::UtilsError::pathError, std::errc::invalid_argument},
        {utils::UtilsError::invalidPathFormat, std::errc::invalid_argument},
        {utils::UtilsError::invalidBase64Input, std::errc::invalid_argument},
        {utils::UtilsError::invalidUrlEncoding, std::errc::invalid_argument},
        {utils::UtilsError::invalidUuidFormat, std::errc::invalid_argument},
        {utils::UtilsError::invalidTimeFormat, std::errc::invalid_argument},
        {utils::UtilsError::timeParseError, std::errc::invalid_argument},
        {utils::UtilsError::pathNotRelative, std::errc::invalid_argument},
        {utils::UtilsError::pathNotAbsolute, std::errc::invalid_argument},
        {utils::UtilsError::basePathNotAncestor, std::errc::invalid_argument},
        {utils::UtilsError::fileTooLarge, std::errc::invalid_argument},
        {utils::UtilsError::analyzerParsingError, std::errc::invalid_argument},
        {utils::UtilsError::outOfRange, std::errc::result_out_of_range},
        {utils::UtilsError::fileAlreadyExists, std::errc::file_exists},
        {utils::UtilsError::directoryNotEmpty, std::errc::directory_not_empty},
        {utils::UtilsError::notADirectory, std::errc::not_a_directory},
        {utils::UtilsError::isADirectory, std::errc::is_a_directory},
        {utils::UtilsError::diskFull, std::errc::no_space_on_device},
        {utils::UtilsError::noSpaceOnDevice, std::errc::no_space_on_device},
        {utils::UtilsError::commandNotFound, std::errc::no_such_process},
        {utils::UtilsError::analyzerProcessNotFound, std::errc::no_such_process},
        {utils::UtilsError::commandExecutionError, std::errc::operation_not_permitted},
        {utils::UtilsError::commandFailed, std::errc::operation_not_permitted},
        {utils::UtilsError::processSpawnFailure, std::errc::operation_not_permitted},
        {utils::UtilsError::analyzerSystemError, std::errc::operation_not_permitted},
        {utils::UtilsError::unsupportedOperation, std::errc::operation_not_supported},
        {utils::UtilsError::traversalStopped, std::errc::operation_not_supported},
        {utils::UtilsError::tempDirectoryError, std::errc::operation_not_supported},
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


// 6. Test to ensure all enum values are covered in the tests
TEST(TypesTest, EnumCompleteness) {
    const std::vector<utils::UtilsError> allErrors = {
        utils::UtilsError::none,
        utils::UtilsError::analyzerPermissionDenied,
        utils::UtilsError::analyzerParsingError,
        utils::UtilsError::analyzerProcessNotFound,
        utils::UtilsError::analyzerSystemError,
        utils::UtilsError::basePathNotAncestor,
        utils::UtilsError::commandExecutionError,
        utils::UtilsError::commandFailed,
        utils::UtilsError::commandNotFound,
        utils::UtilsError::directoryNotEmpty,
        utils::UtilsError::diskFull,
        utils::UtilsError::envVarNotFound,
        utils::UtilsError::fileAlreadyExists,
        utils::UtilsError::fileNotFound,
        utils::UtilsError::fileTooLarge,
        utils::UtilsError::invalidArgument,
        utils::UtilsError::invalidBase64Input,
        utils::UtilsError::invalidPathFormat,
        utils::UtilsError::invalidTimeFormat,
        utils::UtilsError::invalidUrlEncoding,
        utils::UtilsError::invalidUuidFormat,
        utils::UtilsError::ioError,
        utils::UtilsError::isADirectory,
        utils::UtilsError::noSpaceOnDevice,
        utils::UtilsError::notAFile,
        utils::UtilsError::notADirectory,
        utils::UtilsError::outOfRange,
        utils::UtilsError::pathError,
        utils::UtilsError::pathNotAbsolute,
        utils::UtilsError::pathNotRelative,
        utils::UtilsError::permissionDenied,
        utils::UtilsError::permissionDeniedCwd,
        utils::UtilsError::processSpawnFailure,
        utils::UtilsError::tempDirectoryError,
        utils::UtilsError::timeParseError,
        utils::UtilsError::traversalStopped,
        utils::UtilsError::unsupportedOperation,
    };

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
        {utils::UtilsError::fileTooLarge, "File is too large to process"},
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
        {utils::UtilsError::traversalStopped, "Directory traversal stopped by callback"},
        {utils::UtilsError::tempDirectoryError, "Temporary directory error"},
        {utils::UtilsError::outOfRange, "Value out of range"},
        {utils::UtilsError::analyzerProcessNotFound, "Analyzer: Process not found"},
        {utils::UtilsError::analyzerParsingError, "Analyzer: Parsing error"},
        {utils::UtilsError::analyzerSystemError, "Analyzer: System error"},
        {utils::UtilsError::analyzerPermissionDenied, "Analyzer: Permission denied"},
    };

    const std::map<utils::UtilsError, std::errc> mappings = {
        {utils::UtilsError::fileNotFound, std::errc::no_such_file_or_directory},
        {utils::UtilsError::permissionDenied, std::errc::permission_denied},
        {utils::UtilsError::permissionDeniedCwd, std::errc::permission_denied},
        {utils::UtilsError::analyzerPermissionDenied, std::errc::permission_denied},
        {utils::UtilsError::ioError, std::errc::io_error},
        {utils::UtilsError::invalidArgument, std::errc::invalid_argument},
        {utils::UtilsError::pathError, std::errc::invalid_argument},
        {utils::UtilsError::invalidPathFormat, std::errc::invalid_argument},
        {utils::UtilsError::invalidBase64Input, std::errc::invalid_argument},
        {utils::UtilsError::invalidUrlEncoding, std::errc::invalid_argument},
        {utils::UtilsError::invalidUuidFormat, std::errc::invalid_argument},
        {utils::UtilsError::invalidTimeFormat, std::errc::invalid_argument},
        {utils::UtilsError::timeParseError, std::errc::invalid_argument},
        {utils::UtilsError::pathNotRelative, std::errc::invalid_argument},
        {utils::UtilsError::pathNotAbsolute, std::errc::invalid_argument},
        {utils::UtilsError::basePathNotAncestor, std::errc::invalid_argument},
        {utils::UtilsError::fileTooLarge, std::errc::invalid_argument},
        {utils::UtilsError::analyzerParsingError, std::errc::invalid_argument},
        {utils::UtilsError::outOfRange, std::errc::result_out_of_range},
        {utils::UtilsError::fileAlreadyExists, std::errc::file_exists},
        {utils::UtilsError::directoryNotEmpty, std::errc::directory_not_empty},
        {utils::UtilsError::notADirectory, std::errc::not_a_directory},
        {utils::UtilsError::isADirectory, std::errc::is_a_directory},
        {utils::UtilsError::diskFull, std::errc::no_space_on_device},
        {utils::UtilsError::noSpaceOnDevice, std::errc::no_space_on_device},
        {utils::UtilsError::commandNotFound, std::errc::no_such_process},
        {utils::UtilsError::analyzerProcessNotFound, std::errc::no_such_process},
        {utils::UtilsError::commandExecutionError, std::errc::operation_not_permitted},
        {utils::UtilsError::commandFailed, std::errc::operation_not_permitted},
        {utils::UtilsError::processSpawnFailure, std::errc::operation_not_permitted},
        {utils::UtilsError::analyzerSystemError, std::errc::operation_not_permitted},
        {utils::UtilsError::unsupportedOperation, std::errc::operation_not_supported},
        {utils::UtilsError::traversalStopped, std::errc::operation_not_supported},
        {utils::UtilsError::tempDirectoryError, std::errc::operation_not_supported},
    };

    // Check that every enum value is in the message map
    for (utils::UtilsError err : allErrors) {
        EXPECT_TRUE(errorMessages.count(err))
            << "Error " << static_cast<int>(err) << " is missing from the MessageCorrectness test.";
    }

    // Check that every enum value that should be mapped is in the mappings map
    // (excluding 'none', 'notAFile', etc. which don't map to a std::errc)
    std::set<utils::UtilsError> unmappedErrors = {
        utils::UtilsError::none,
        utils::UtilsError::notAFile, // No direct equivalent in std::errc
        utils::UtilsError::envVarNotFound, // No direct equivalent in std::errc
    };

    for (utils::UtilsError err : allErrors) {
        if (!unmappedErrors.contains(err)) {
            EXPECT_TRUE(mappings.count(err))
                << "Error " << static_cast<int>(err) << " is missing from the ErrorConditionMappings test.";
        }
    }
}
