#pragma once

#include <cstdint>
#include <string_view>

/// @file
/// @brief Error codes returned by every xvfs function that can fail.
///
/// xvfs never throws, asserts or prints. A function that can fail returns an
/// xvfs::error_code, or an xvfs::result that holds either a value or an error_code.
///
/// @code
/// const xvfs::error_code error = vfs.add_wildcard("content:", "/game/content");
/// if (error != xvfs::error_code::none)
/// {
///     report(xvfs::get_error_message(error));
/// }
/// @endcode
///
/// Threading: everything in this file is safe to call from any thread.

namespace xvfs
{
    /// @brief What went wrong. xvfs::error_code::none means success.
    enum class error_code : uint8_t
    {
        none,              ///< Success.
        empty_path,        ///< The path to resolve was empty.
        empty_wildcard,    ///< context::add_wildcard was given an empty wildcard.
        no_wildcards,      ///< The context has no wildcards, so nothing can be resolved.
        buffer_too_small,  ///< The caller's buffer cannot hold the resolved path.
        not_a_file,        ///< The resolved path does not name a regular file.
        open_failed,       ///< The file exists but could not be opened (permissions, locks).
        size_query_failed, ///< The size of an opened file could not be read.
        read_failed,       ///< Reading stopped before every byte of the file was read.
        filesystem_error,  ///< The operating system refused a filesystem operation.
    };

    /// @brief Describes an error in a short English sentence, for logs and tools.
    /// @param error The error to describe.
    /// @return A view of a string literal; it stays valid for the whole program.
    [[nodiscard]] std::string_view get_error_message(error_code error);
} // namespace xvfs
