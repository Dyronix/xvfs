#pragma once

#include "xvfs/context.h"
#include "xvfs/error.h"
#include "xvfs/input_file.h"
#include "xvfs/result.h"

#include <concepts>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

/// @file
/// @brief Whole-file reads and file queries through a context.
///
/// @code
/// const xvfs::result<std::string> text = xvfs::read_text_file(vfs, "content:/config/game.json");
/// const xvfs::result<std::vector<std::byte>> bytes = xvfs::read_binary_file(vfs, "content:/maps/harbor.map");
/// const bool ready = xvfs::is_file(vfs, "content:/maps/harbor.map");
/// @endcode
///
/// Ownership: the returned containers belong to the caller and use the caller's allocator.
/// Threading: safe to call from many threads at once with the same const context.
/// Failure: every read returns a result and logs the details through the context's callback.

namespace xvfs
{
    /// @brief Reads a whole file as bytes.
    /// @tparam Allocator The allocator of the returned vector; its value type is std::byte.
    /// @param owner The context that resolves the path.
    /// @param path The content path.
    /// @param allocator The allocator instance the vector is made with.
    /// @return The bytes, or an error from open_input_file() or input_file::read_into().
    template <typename Allocator = std::allocator<std::byte>>
        requires std::same_as<typename Allocator::value_type, std::byte>
    [[nodiscard]] result<std::vector<std::byte, Allocator>> read_binary_file(const context& owner, std::string_view path, const Allocator& allocator = Allocator());

    /// @brief Reads a whole file as text, byte for byte (no newline conversion).
    /// @tparam Allocator The allocator of the returned string; its value type is char.
    /// @param owner The context that resolves the path.
    /// @param path The content path.
    /// @param allocator The allocator instance the string is made with.
    /// @return The text, or an error from open_input_file() or input_file::read_into().
    template <typename Allocator = std::allocator<char>>
        requires std::same_as<typename Allocator::value_type, char>
    [[nodiscard]] result<std::basic_string<char, std::char_traits<char>, Allocator>> read_text_file(const context& owner, std::string_view path, const Allocator& allocator = Allocator());

    /// @brief Says whether a content path names an existing regular file.
    /// @param owner The context that resolves the path.
    /// @param path The content path.
    /// @return True for a regular file. False for a directory, a missing path or a resolve
    ///         error (the error is logged).
    bool is_file(const context& owner, std::string_view path);

    /// @brief Says whether a content path names an existing directory.
    /// @param owner The context that resolves the path.
    /// @param path The content path.
    /// @return True for a directory. False otherwise (a resolve error is logged).
    bool is_directory(const context& owner, std::string_view path);

    /// @brief Creates a directory and any missing parents. Existing directories are fine.
    /// @param owner The context that resolves the path.
    /// @param path The content path of the directory.
    /// @return error_code::none, the resolve error, or error_code::filesystem_error.
    [[nodiscard]] error_code create_directories(const context& owner, std::string_view path);

    template <typename Allocator>
        requires std::same_as<typename Allocator::value_type, std::byte>
    result<std::vector<std::byte, Allocator>> read_binary_file(const context& owner, std::string_view path, const Allocator& allocator)
    {
        using bytes = std::vector<std::byte, Allocator>;

        result<input_file> opened = open_input_file(owner, path);
        if (!opened.has_value())
        {
            return result<bytes>(opened.get_error());
        }

        input_file file = std::move(opened).get_value();
        bytes buffer(static_cast<std::size_t>(file.get_size()), allocator);

        const error_code error = file.read_into(std::span<std::byte>(buffer));
        if (error != error_code::none)
        {
            return result<bytes>(error);
        }

        return result<bytes>(std::move(buffer));
    }

    template <typename Allocator>
        requires std::same_as<typename Allocator::value_type, char>
    result<std::basic_string<char, std::char_traits<char>, Allocator>> read_text_file(const context& owner, std::string_view path, const Allocator& allocator)
    {
        using text = std::basic_string<char, std::char_traits<char>, Allocator>;

        result<input_file> opened = open_input_file(owner, path);
        if (!opened.has_value())
        {
            return result<text>(opened.get_error());
        }

        input_file file = std::move(opened).get_value();
        text buffer(static_cast<std::size_t>(file.get_size()), '\0', allocator);

        const error_code error = file.read_into(std::as_writable_bytes(std::span<char>(buffer)));
        if (error != error_code::none)
        {
            return result<text>(error);
        }

        return result<text>(std::move(buffer));
    }
} // namespace xvfs
