#include "xvfs/input_file.h"

#include "log_message.h"
#include "xvfs/context.h"
#include "xvfs/native_path.h"

#include <filesystem>
#include <ios>
#include <system_error>
#include <utility>

namespace xvfs
{
    namespace
    {
        // std::istream::tellg reports failure with this position.
        constexpr std::streamoff k_invalid_position = -1;
    } // namespace

    //-------------------------------------------------------------------------
    input_file::input_file(const context& owner, std::string resolved_path, std::ifstream stream, uint64_t size)
        : m_context(&owner)
        , m_path(std::move(resolved_path))
        , m_stream(std::move(stream))
        , m_size(size)
    {
    }

    //-------------------------------------------------------------------------
    uint64_t input_file::get_size() const
    {
        return m_size;
    }

    //-------------------------------------------------------------------------
    std::string_view input_file::get_path() const
    {
        return m_path;
    }

    //-------------------------------------------------------------------------
    error_code input_file::read_into(std::span<std::byte> destination)
    {
        m_stream.clear();
        m_stream.seekg(0, std::ios::beg);

        // convention-exception: reinterpret_cast at the byte boundary of std::istream::read.
        char* const bytes = reinterpret_cast<char*>(destination.data());
        const auto count = static_cast<std::streamsize>(destination.size());
        if (m_stream.read(bytes, count))
        {
            return error_code::none;
        }

        const std::string_view reason = m_stream.bad() ? "an I/O error" : "the end of the file";
        detail::log_message(m_context->get_log_callback(), log_level::error, {"[xvfs] Reading '", m_path, "' stopped at ", reason});

        return error_code::read_failed;
    }

    //-------------------------------------------------------------------------
    result<input_file> open_input_file(const context& owner, std::string_view path)
    {
        const log_callback& log = owner.get_log_callback();

        result<std::string> resolved = owner.resolve_path(path);
        if (!resolved.has_value())
        {
            return result<input_file>(resolved.get_error());
        }

        std::string real_path = std::move(resolved).get_value();

        const result<std::filesystem::path> native = make_native_path(real_path);
        if (!native.has_value())
        {
            detail::log_message(log, log_level::error, {"[xvfs] The path of '", path, "' is not valid UTF-8"});
            return result<input_file>(native.get_error());
        }

        // An std::ifstream opens a directory on some platforms, so ask the filesystem first.
        std::error_code status_error;
        if (!std::filesystem::is_regular_file(native.get_value(), status_error))
        {
            detail::log_message(log, log_level::error, {"[xvfs] File '", path, "' at '", real_path, "' was not found"});
            return result<input_file>(error_code::not_a_file);
        }

        std::ifstream stream(native.get_value(), std::ios::binary | std::ios::ate);
        if (!stream.is_open())
        {
            detail::log_message(log, log_level::error, {"[xvfs] File '", path, "' at '", real_path, "' could not be opened"});
            return result<input_file>(error_code::open_failed);
        }

        const std::streamoff end = stream.tellg();
        if (end == k_invalid_position)
        {
            detail::log_message(log, log_level::error, {"[xvfs] The size of '", real_path, "' could not be read"});
            return result<input_file>(error_code::size_query_failed);
        }

        detail::log_message(log, log_level::debug, {"[xvfs] Opened '", real_path, "'"});

        const auto size = static_cast<uint64_t>(end);

        return result<input_file>(input_file(owner, std::move(real_path), std::move(stream), size));
    }
} // namespace xvfs
