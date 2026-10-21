#pragma once

#include <string>
#include <string_view>

/// @file
/// @brief Path text helpers shared by file_info and directory_info.
///
/// @code
/// const std::string path = xvfs::make_normalized_path("content:\\maps\\harbor.map");
/// // path == "content:/maps/harbor.map"
/// @endcode
///
/// xvfs accepts both `/` and `\` as separators and always writes `/`.
/// Threading: pure functions, safe from any thread. Failure: these functions cannot fail.

namespace xvfs
{
    /// @brief Copies a path and turns every `\` into `/`. Nothing else changes.
    /// @param path Any path text.
    /// @return The normalized copy.
    std::string make_normalized_path(std::string_view path);
} // namespace xvfs
