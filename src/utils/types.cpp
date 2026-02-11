// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Eser KUBALI

#include "utils/types.h"
#include <charconv> // Required for std::from_chars
#include <cstdint>  // Required for uint32_t

namespace utils {

    template <typename TInt>
    std::optional<TInt> parseIntegerNoThrow(std::string_view text, int base) {
        TInt value{};
        const char* begin = text.data();
        const char* end = begin + text.size();
        const auto [ptr, ec] = std::from_chars(begin, end, value, base);
        if (ec != std::errc{} || ptr != end) {
            return std::nullopt;
        }
        return value;
    }

    // Explicit instantiations for common integer types to avoid linker errors.
    // This allows the template function to be defined in a .cpp file.
    template std::optional<int> parseIntegerNoThrow<int>(std::string_view text, int base);
    template std::optional<long> parseIntegerNoThrow<long>(std::string_view text, int base);
    template std::optional<short> parseIntegerNoThrow<short>(std::string_view text, int base); // Added for testing
    template std::optional<unsigned char> parseIntegerNoThrow<unsigned char>(std::string_view text, int base); // Added for testing
    template std::optional<uint32_t> parseIntegerNoThrow<uint32_t>(std::string_view text, int base);

} // namespace utils