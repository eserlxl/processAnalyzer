// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/file.h"
#include <cerrno>
#include <fcntl.h>
#include <fstream>
#include <iterator>
#include <random>
#include <unistd.h>
#include <vector>

namespace utils {

// Helper to generate random string for temp files
namespace {
constexpr size_t tempFileSuffixLen = 6;
constexpr size_t tempCreationRetries = 10;
constexpr size_t randomNameLen = 16;

Result<void> writeToPath(const ::std::filesystem::path& path, const void* data,
                         size_t size, ::std::ios_base::openmode mode) {
  ::std::ofstream file(path, mode);
  if (!file.is_open()) {
    return ::std::unexpected(make_error_code(UtilsError::ioError));
  }
  if (size >
      static_cast<size_t>(::std::numeric_limits<::std::streamsize>::max())) {
    return ::std::unexpected(make_error_code(UtilsError::fileTooLarge));
  }
  if (file.write(static_cast<const char*>(data),
                 static_cast<::std::streamsize>(size))) {
    return {};
  }
  return ::std::unexpected(make_error_code(UtilsError::ioError));
}

} // namespace

static ::std::string generateRandomString(size_t length) {
  static constexpr ::std::string_view charset = "0123456789"
                                                "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                                "abcdefghijklmnopqrstuvwxyz";
  static ::std::mt19937 rg{::std::random_device{}()};
  static ::std::uniform_int_distribution<::std::string::size_type> pick(
      0, charset.size() - 1);

  ::std::string s;
  s.reserve(length);
  for (size_t i = 0; i < length; ++i)
    s += charset[pick(rg)];
  return s;
}

Result<::std::vector<::std::byte>>
readBinaryFile(const ::std::filesystem::path& path) {
  ::std::error_code ec;
  if (!::std::filesystem::exists(path, ec)) {
    if (ec)
      return ::std::unexpected(ec);
    return ::std::unexpected(make_error_code(UtilsError::fileNotFound));
  }

  ::std::ifstream file(path, ::std::ios::binary | ::std::ios::ate);
  if (!file.is_open()) {
    return ::std::unexpected(make_error_code(UtilsError::ioError));
  }

  auto fileSize = file.tellg();
  if (fileSize < 0) {
    return ::std::unexpected(make_error_code(UtilsError::ioError));
  }
  if (static_cast<uintmax_t>(fileSize) >
      ::std::numeric_limits<size_t>::max()) {
    return ::std::unexpected(make_error_code(UtilsError::fileTooLarge));
  }

  ::std::vector<::std::byte> buffer(static_cast<size_t>(fileSize));
  file.seekg(0, ::std::ios::beg);

  if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
    return ::std::unexpected(make_error_code(UtilsError::ioError));
  }

  return buffer;
}

Result<void> writeBinaryFile(const ::std::filesystem::path& path,
                             ::std::span<const ::std::byte> content) {
  return writeToPath(path, content.data(), content.size(),
                     ::std::ios::out | ::std::ios::trunc | ::std::ios::binary);
}

template <typename Writer>
Result<void> doAtomicWrite(const ::std::filesystem::path& path, Writer writer) {
  auto parent = path.parent_path();
  if (parent.empty())
    parent = ".";

  ::std::error_code ec;
  if (!::std::filesystem::exists(parent, ec)) {
    if (ec)
      return ::std::unexpected(ec); // Propagate actual error if exists() failed
    return ::std::unexpected(make_error_code(UtilsError::fileNotFound));
  }
  if (!::std::filesystem::is_directory(parent, ec)) {
    if (ec)
      return ::std::unexpected(
          ec); // Propagate actual error if is_directory() failed
    return ::std::unexpected(make_error_code(UtilsError::notADirectory));
  }

  auto tempPath = parent / (path.filename().string() + "." +
                            generateRandomString(tempFileSuffixLen) + ".tmp");

  auto writeResult = writer(tempPath);
  if (!writeResult) {
    ::std::filesystem::remove(tempPath, ec); // Try cleanup
    return writeResult;
  }

  ::std::filesystem::rename(tempPath, path, ec);
  if (ec) {
    ::std::filesystem::remove(tempPath, ec); // Try cleanup
    return ::std::unexpected(ec);
  }
  return {};
}

Result<void> writeTextFileAtomic(const ::std::filesystem::path& path,
                                 ::std::string_view content) {
  return doAtomicWrite(path, [&](const ::std::filesystem::path& tempPath) {
    return writeTextFile(tempPath, content);
  });
}

Result<void>
writeBinaryFileAtomic(const ::std::filesystem::path& path,
                      ::std::span<const ::std::byte> content) {
  return doAtomicWrite(path, [&](const ::std::filesystem::path& tempPath) {
    return writeBinaryFile(tempPath, content);
  });
}
Result<void>
appendToBinaryFile(const ::std::filesystem::path& path,
                   ::std::span<const ::std::byte> content) {
  return writeToPath(path, content.data(), content.size(),
                     ::std::ios::out | ::std::ios::app | ::std::ios::binary);
}

Result<::std::filesystem::path>
createTemporaryFile(::std::string_view prefix, ::std::string_view suffix) {
  ::std::error_code ec;
  auto tempDir = ::std::filesystem::temp_directory_path(ec);
  if (ec) {
    return ::std::unexpected(ec); // Error getting temp path itself
  }
  if (!::std::filesystem::exists(tempDir, ec) ||
      !::std::filesystem::is_directory(tempDir, ec)) {
    if (ec)
      return ::std::unexpected(ec); // Error checking existence or type
    return ::std::unexpected(make_error_code(UtilsError::tempDirectoryError));
  }
  ::std::filesystem::path tempPath;

  // Try a few times to generate a unique name and create atomically.
  for (size_t i = 0; i < tempCreationRetries; ++i) {
    ::std::string name;
    name.reserve(prefix.size() + randomNameLen + suffix.size());
    name.append(prefix);
    name.append(generateRandomString(randomNameLen));
    name.append(suffix);
    tempPath = tempDir / name;

    const int fd = ::open(tempPath.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
    if (fd >= 0) {
      ::close(fd);
      return tempPath;
    }

    if (errno == EEXIST) {
      continue;
    }

    return ::std::unexpected(::std::error_code(errno, ::std::generic_category()));
  }
  return ::std::unexpected(make_error_code(UtilsError::ioError));
}

Result<::std::filesystem::path>
createTemporaryDirectory(::std::string_view prefix) {
  ::std::error_code ec;
  auto tempDir = ::std::filesystem::temp_directory_path(ec);
  if (ec) {
    return ::std::unexpected(ec); // Error getting temp path itself
  }
  if (!::std::filesystem::exists(tempDir, ec) ||
      !::std::filesystem::is_directory(tempDir, ec)) {
    if (ec)
      return ::std::unexpected(ec); // Error checking existence or type
    return ::std::unexpected(make_error_code(UtilsError::tempDirectoryError));
  }
  ::std::filesystem::path path;

  for (size_t i = 0; i < tempCreationRetries; ++i) {
    ::std::string name;
    name.reserve(prefix.size() + randomNameLen);
    name.append(prefix);
    name.append(generateRandomString(randomNameLen));
    path = tempDir / name;
    if (::std::filesystem::create_directory(path, ec)) {
      return path;
    }
    // If creation failed and it's not due to file_exists (which implies a
    // race on name) then it's a persistent error, so return it immediately.
    if (ec && ec != ::std::make_error_code(::std::errc::file_exists)) {
      return ::std::unexpected(ec);
    }
    // If ec is set to file_exists, we continue the loop to try another name.
    // If ec is not set, it means create_directory returned false for some
    // other reason but didn't set a specific error (unlikely for
    // create_directory but possible for generic errors or custom filesystem
    // implementations), in which case we continue trying.
  }
  return ::std::unexpected(
      make_error_code(UtilsError::ioError)); // All retries failed or
                                             // unspecified error
}

Result<::std::string> readTextFile(const ::std::filesystem::path& path) {
  ::std::error_code ec;
  if (!::std::filesystem::exists(path, ec)) {
    if (ec)
      return ::std::unexpected(ec);
    return ::std::unexpected(make_error_code(UtilsError::fileNotFound));
  }

  ::std::ifstream file(path);
  if (!file.is_open()) {
    return ::std::unexpected(make_error_code(UtilsError::ioError));
  }

  ::std::string content((::std::istreambuf_iterator<char>(file)),
                        ::std::istreambuf_iterator<char>());

  if (file.bad()) {
    return ::std::unexpected(make_error_code(UtilsError::ioError));
  }

  return content;
}

Result<void> writeTextFile(const ::std::filesystem::path& path,
                           ::std::string_view content) {
  // Open in binary mode to prevent platform-specific newline translation (e.g.,
  // \n -> \r\n on Windows). This ensures byte-exact writing of text content as
  // provided.
  return writeToPath(path, content.data(), content.size(),
                     ::std::ios::out | ::std::ios::trunc | ::std::ios::binary);
}

Result<void> appendToFile(const ::std::filesystem::path& path,
                          ::std::string_view content) {
  // Open in binary mode to prevent platform-specific newline translation (e.g.,
  // \n -> \r\n on Windows). This ensures byte-exact appending of text content
  // as provided.
  return writeToPath(path, content.data(), content.size(),
                     ::std::ios::out | ::std::ios::app | ::std::ios::binary);
}

Result<::std::vector<::std::string>>
readLines(const ::std::filesystem::path& path) {
  ::std::error_code ec;
  if (!::std::filesystem::exists(path, ec)) {
    if (ec)
      return ::std::unexpected(ec);
    return ::std::unexpected(make_error_code(UtilsError::fileNotFound));
  }
  if (!::std::filesystem::is_regular_file(path, ec)) {
    if (ec)
      return ::std::unexpected(ec);
    return ::std::unexpected(make_error_code(UtilsError::ioError));
  }

  ::std::ifstream file(path);
  if (!file.is_open()) {
    return ::std::unexpected(make_error_code(UtilsError::ioError));
  }

  ::std::vector<::std::string> lines;
  ::std::string line;
  while (::std::getline(file, line)) {
    lines.push_back(::std::move(line));
  }

  if (file.bad() || (file.fail() && !file.eof())) {
    return ::std::unexpected(make_error_code(UtilsError::ioError));
  }
  return lines;
}

} // namespace utils
