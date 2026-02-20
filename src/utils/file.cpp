// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/file.h"
#include <cerrno>
#include <fstream>
#include <iterator>
#include <vector>

namespace utils {

// Helper to generate random string for temp files
namespace {

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
  return writeToPath(path, content.data(), content.size(),
                     ::std::ios::out | ::std::ios::app | ::std::ios::binary);
}

} // namespace utils

