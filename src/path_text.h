#pragma once

#include <cstddef>
#include <string>
#include <string_view>

// Internal to xvfs (not installed): the separator rules shared by the path parsers and the
// context. Small and called per character, so the bodies stay in the header (P42 carve-out).
// Threading: pure functions, safe from any thread.

namespace xvfs::detail
{
    // xvfs writes this separator in every path it returns.
    constexpr char k_separator = '/';

    // Accepted in input and turned into k_separator.
    constexpr char k_windows_separator = '\\';

    // Both separators, for std::string_view::find_last_of.
    constexpr std::string_view k_separators = "/\\";

    constexpr bool is_separator(char c)
    {
        return c == k_separator || c == k_windows_separator;
    }

    constexpr char to_forward_slash(char c)
    {
        return is_separator(c) ? k_separator : c;
    }

    // std::string_view::npos when the path has no separator.
    constexpr std::size_t find_last_separator(std::string_view path)
    {
        return path.find_last_of(k_separators);
    }

    // Joins directory + "/" + name + extension (no "/" for an empty directory or one that
    // already ends in a separator) with every separator written as k_separator. One allocation.
    std::string make_joined_path(std::string_view directory, std::string_view name, std::string_view extension);
} // namespace xvfs::detail
