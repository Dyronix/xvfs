#pragma once

#include "xvfs/error.h"
#include "xvfs/log.h"
#include "xvfs/result.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

/// @file
/// @brief xvfs::context, the table of wildcards that turns content paths into real paths.
///
/// A wildcard is a short name such as `content:` that stands for a real directory.
/// Code that loads data names files relative to a wildcard, and only the context knows
/// where that directory really is.
///
/// @code
/// xvfs::context vfs;
/// vfs.set_log_callback(my_log_callback);
/// const xvfs::error_code added = vfs.add_wildcard("content:", "/home/me/game/content");
///
/// const xvfs::result<std::string> path = vfs.resolve_path("content:/maps/harbor.map");
/// // path.get_value() == "/home/me/game/content/maps/harbor.map"
/// @endcode
///
/// Rules:
/// - A wildcard matches only at the start of a path. When several match, the longest wins.
/// - The text after the wildcard is joined to the wildcard's directory with exactly one `/`.
/// - A path that starts with no wildcard is returned unchanged (absolute paths pass through).
/// - Every `\` becomes `/` in the result, in the wildcard's directory and in the path.
///
/// Ownership: the caller owns the context. Nothing in xvfs is global; make as many contexts
/// as you need (one per test, one per tool).
///
/// Threading: the standard library contract. Const member functions (resolve_path,
/// resolve_path_into, get_log_callback) are safe to call from many threads at once.
/// Non-const member functions (add_wildcard, set_log_callback) need exclusive access:
/// register every wildcard before the context is shared, then share it as `const context&`.
/// To change the table while other threads resolve, build a new context and swap it in.
///
/// Failure: every function that can fail returns an error_code or a result, and also logs
/// the details through the log callback when one is set.

namespace xvfs
{
    /// @brief Maps wildcard prefixes to real directories. See the file comment for the rules.
    class context
    {
    public:
        /// @brief Makes an empty context: no wildcards and no log callback.
        context();

        /// @brief Sets the function that receives log messages. Not thread safe.
        /// @param callback The new callback; an empty one silences the context.
        void set_log_callback(log_callback callback);

        /// @brief Gives the current log callback, so file functions can log with it.
        /// @return The callback; it may be empty.
        const log_callback& get_log_callback() const;

        /// @brief Adds a wildcard, or replaces the directory of an existing one. Not thread safe.
        ///
        /// End a wildcard with a character that cannot start a directory name, such as `:`,
        /// so `content:` never matches a path like `contents/a.txt`.
        /// @param wildcard The prefix to match, for example `content:`. Must not be empty.
        /// @param directory The real directory it stands for. May be empty (current directory).
        /// @return error_code::none, or error_code::empty_wildcard.
        [[nodiscard]] error_code add_wildcard(std::string_view wildcard, std::string_view directory);

        /// @brief Resolves a content path into a real path. Thread safe.
        ///
        /// Allocates at most once: the returned string, sized exactly.
        /// @param path The content path, for example `content:/maps/harbor.map`.
        /// @return The real path, or error_code::empty_path or error_code::no_wildcards.
        [[nodiscard]] result<std::string> resolve_path(std::string_view path) const;

        /// @brief Resolves a content path into a buffer the caller owns. Thread safe, never allocates.
        ///
        /// For hot paths that must not allocate. The result is not null-terminated.
        /// @param path The content path.
        /// @param buffer Where the real path is written. Nothing is written when it is too small.
        /// @return A view of the written part of @p buffer, or error_code::empty_path,
        ///         error_code::no_wildcards or error_code::buffer_too_small.
        [[nodiscard]] result<std::string_view> resolve_path_into(std::string_view path, std::span<char> buffer) const;

    private:
        struct wildcard_entry
        {
            std::string name;
            std::string directory;
        };

        // Null when no wildcard is a prefix of the path.
        const wildcard_entry* find_longest_match(std::string_view path) const;

        // Checks the preconditions of a resolve and logs a failure.
        error_code check_resolvable(std::string_view path) const;

        std::vector<wildcard_entry> m_wildcards;
        log_callback m_log_callback;
    };
} // namespace xvfs
