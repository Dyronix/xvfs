#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

/// @file
/// @brief The optional log callback. xvfs never prints; it hands messages to this callback.
///
/// @code
/// vfs.set_log_callback([](xvfs::log_level level, std::string_view message) { my_log(level, message); });
/// @endcode
///
/// Ownership: the message view is only valid during the call; copy it to keep it.
/// Threading: xvfs calls the callback from whichever thread called xvfs, possibly from
/// several threads at once, so the callback must be thread safe.

namespace xvfs
{
    /// @brief How important a log message is.
    enum class log_level : uint8_t
    {
        debug,   ///< Normal operation worth tracing, such as a file being opened.
        info,    ///< A configuration change, such as a wildcard being added.
        warning, ///< A call that cannot work yet, such as resolving before any wildcard exists.
        error,   ///< A call that failed; the same call also returns an error_code.
    };

    /// @brief Receives every message xvfs produces. An empty callback drops them.
    /// @param level How important the message is.
    /// @param message One line of text without a trailing newline.
    using log_callback = std::function<void(log_level level, std::string_view message)>;
} // namespace xvfs
