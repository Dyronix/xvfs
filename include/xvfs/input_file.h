#pragma once

#include "xvfs/error.h"
#include "xvfs/result.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <span>
#include <string>
#include <string_view>

/// @file
/// @brief xvfs::input_file, an open file ready to be read in one go.
///
/// Most code calls read_binary_file or read_text_file from xvfs/fileio.h. Use input_file
/// directly to read into memory you already own.
///
/// @code
/// xvfs::result<xvfs::input_file> opened = xvfs::open_input_file(vfs, "content:/maps/harbor.map");
/// if (opened.has_value())
/// {
///     xvfs::input_file file = std::move(opened).get_value();
///     std::vector<std::byte> bytes(file.get_size());
///     const xvfs::error_code error = file.read_into(bytes);
/// }
/// @endcode
///
/// Ownership: input_file owns the open file and closes it when destroyed. It keeps a
/// pointer to the context it was opened with (for logging), so that context must outlive it.
/// Threading: one input_file belongs to one thread at a time; different files may be read
/// from different threads at once.
/// Failure: read_into returns an error_code and logs through the context's log callback.

namespace xvfs
{
    class context;

    /// @brief An open binary file. Move-only. Made by open_input_file().
    class input_file
    {
    public:
        /// @brief Takes over an open stream. Called by open_input_file(); rarely needed directly.
        /// @param owner The context whose log callback reports read failures; must outlive the file.
        /// @param resolved_path The real path, kept for log messages.
        /// @param stream An open stream positioned anywhere.
        /// @param size The size of the file in bytes.
        input_file(const context& owner, std::string resolved_path, std::ifstream stream, uint64_t size);

        /// @brief Gives the size of the file when it was opened.
        /// @return The size in bytes.
        uint64_t get_size() const;

        /// @brief Gives the real path the file was opened from.
        /// @return A view that lives as long as this object.
        std::string_view get_path() const;

        /// @brief Reads the start of the file into memory the caller owns.
        ///
        /// Reads exactly destination.size() bytes from the start of the file.
        /// @param destination Where the bytes go; usually get_size() bytes long.
        /// @return error_code::none, or error_code::read_failed when fewer bytes were available.
        [[nodiscard]] error_code read_into(std::span<std::byte> destination);

    private:
        const context* m_context;
        std::string m_path;
        std::ifstream m_stream;
        uint64_t m_size;
    };

    /// @brief Resolves a content path and opens the file it names.
    /// @param owner The context that resolves the path and logs; it must outlive the file.
    /// @param path The content path, for example `content:/maps/harbor.map`.
    /// @return The open file, or the resolve error, error_code::not_a_file,
    ///         error_code::open_failed or error_code::size_query_failed.
    [[nodiscard]] result<input_file> open_input_file(const context& owner, std::string_view path);
} // namespace xvfs
