#pragma once

#include <algorithm>
#include <cctype>
#include <string>

// ES.5: Use standard algorithms for common operations
// C++ Core Guidelines: Simple string utilities for case-insensitive operations

namespace StringUtils {
// Convert string to lowercase
// ES.5: Use std::transform with std::tolower
inline std::string ToLower(const std::string &str) {
    std::string result;
    result.reserve(str.size());
    std::transform(str.begin(), str.end(), std::back_inserter(result),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Case-insensitive substring search
// I.12: Check preconditions (empty needle)
inline bool ContainsIgnoreCase(const std::string &haystack,
                               const std::string &needle) {
    if (needle.empty()) {
        return true; // Empty needle matches everything
    }
    const std::string haystack_lower = ToLower(haystack);
    const std::string needle_lower = ToLower(needle);
    return haystack_lower.find(needle_lower) != std::string::npos;
}
} // namespace StringUtils
