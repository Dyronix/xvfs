#include "xvfs/fileio.h"

#include "log_message.h"
#include "xvfs/native_path.h"

#include <filesystem>
#include <system_error>

namespace xvfs
{
    namespace
    {
        //-------------------------------------------------------------------------
        result<std::filesystem::path> resolve_native_path(const context& owner, std::string_view path)
        {
            const result<std::string> resolved = owner.resolve_path(path);
            if (!resolved.has_value())
            {
                return result<std::filesystem::path>(resolved.get_error());
            }

            result<std::filesystem::path> native = make_native_path(resolved.get_value());
            if (!native.has_value())
            {
                detail::log_message(owner.get_log_callback(), log_level::error, {"[xvfs] The path of '", path, "' is not valid UTF-8"});
            }

            return native;
        }
    } // namespace

    //-------------------------------------------------------------------------
    bool is_file(const context& owner, std::string_view path)
    {
        const result<std::filesystem::path> native = resolve_native_path(owner, path);
        if (!native.has_value())
        {
            return false;
        }

        std::error_code error;

        return std::filesystem::is_regular_file(native.get_value(), error);
    }

    //-------------------------------------------------------------------------
    bool is_directory(const context& owner, std::string_view path)
    {
        const result<std::filesystem::path> native = resolve_native_path(owner, path);
        if (!native.has_value())
        {
            return false;
        }

        std::error_code error;

        return std::filesystem::is_directory(native.get_value(), error);
    }

    //-------------------------------------------------------------------------
    error_code create_directories(const context& owner, std::string_view path)
    {
        const result<std::filesystem::path> native = resolve_native_path(owner, path);
        if (!native.has_value())
        {
            return native.get_error();
        }

        // create_directories reports "already there" as success with no error set.
        std::error_code error;
        std::filesystem::create_directories(native.get_value(), error);
        if (error)
        {
            const std::string real_path = make_utf8_path(native.get_value());
            const std::string reason = error.message();
            detail::log_message(owner.get_log_callback(), log_level::error, {"[xvfs] Could not create directory '", real_path, "': ", reason});
            return error_code::filesystem_error;
        }

        return error_code::none;
    }
} // namespace xvfs
