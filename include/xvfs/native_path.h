#pragma once

#include "xvfs/result.h"

#include <filesystem>
#include <string>
#include <string_view>

/// @file
/// @brief Converts xvfs path text (UTF-8) to and from std::filesystem::path.
///
/// Every path in the xvfs API is UTF-8 text in a `char` string. xvfs uses these functions
/// wherever it touches the filesystem, and callers use them to hand real directories to
/// context::add_wildcard or to open a resolved path with other libraries.
///
/// @code
/// const std::string root = xvfs::make_utf8_path(std::filesystem::current_path());
/// const xvfs::error_code added = vfs.add_wildcard("local:", root);
///
/// const xvfs::result<std::filesystem::path> native = xvfs::make_native_path(vfs.resolve_path("local:/a.txt").get_value());
/// @endcode
///
/// The conversion goes through std::u8string, so std::filesystem::path decodes the text as
/// UTF-8 on every platform (to UTF-16 on Windows, unchanged bytes on Linux and PS5). The
/// deprecated std::filesystem::u8path is not used.
///
/// Threading: pure functions, safe from any thread.
/// Failure: make_native_path rejects text that is not well-formed UTF-8 with
/// error_code::invalid_utf8, the same way on every platform.

namespace xvfs
{
    /// @brief Says whether text is well-formed UTF-8 (RFC 3629).
    ///
    /// Rejects overlong forms, UTF-16 surrogates (U+D800 to U+DFFF), code points above
    /// U+10FFFF and truncated sequences. Empty text is valid.
    /// @param text The text to check.
    /// @return True when the text is well-formed UTF-8.
    bool is_valid_utf8(std::string_view text);

    /// @brief Turns UTF-8 path text into a std::filesystem::path.
    /// @param utf8_path The path text, UTF-8.
    /// @return The native path, or error_code::invalid_utf8.
    [[nodiscard]] result<std::filesystem::path> make_native_path(std::string_view utf8_path);

    /// @brief Turns a std::filesystem::path into UTF-8 path text with `/` separators.
    ///
    /// On Windows the UTF-16 path is encoded as UTF-8. On Linux and PS5 the bytes are
    /// returned as they are, so a name that is not UTF-8 on disk stays not UTF-8.
    /// @param native_path The path to convert.
    /// @return The UTF-8 text.
    std::string make_utf8_path(const std::filesystem::path& native_path);
} // namespace xvfs
