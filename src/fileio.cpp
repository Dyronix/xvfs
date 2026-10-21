#include "xvfs/fileio.h"

#include "log_message.h"

#include <filesystem>
#include <system_error>

namespace xvfs
{
    //-------------------------------------------------------------------------
    bool is_file(const context& owner, std::string_view path)
    {
        const result<std::string> resolved = owner.resolve_path(path);
        if (!resolved.has_value())
        {
            return false;
        }

        std::error_code error;

        return std::filesystem::is_regular_file(resolved.get_value(), error);
    }

    //-------------------------------------------------------------------------
    bool is_directory(const context& owner, std::string_view path)
    {
        const result<std::string> resolved = owner.resolve_path(path);
        if (!resolved.has_value())
        {
            return false;
        }

        std::error_code error;

        return std::filesystem::is_directory(resolved.get_value(), error);
    }

    //-------------------------------------------------------------------------
    error_code create_directories(const context& owner, std::string_view path)
    {
        const result<std::string> resolved = owner.resolve_path(path);
        if (!resolved.has_value())
        {
            return resolved.get_error();
        }

        const std::string& real_path = resolved.get_value();

        // create_directories reports "already there" as success with no error set.
        std::error_code error;
        std::filesystem::create_directories(real_path, error);
        if (error)
        {
            const std::string reason = error.message();
            detail::log_message(owner.get_log_callback(), log_level::error, {"[xvfs] Could not create directory '", real_path, "': ", reason});
            return error_code::filesystem_error;
        }

        return error_code::none;
    }
} // namespace xvfs
