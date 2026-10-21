#include "xvfs/directory_info.h"

#include "path_text.h"

#include <cstddef>

namespace xvfs
{
    namespace
    {
        // The two parts of a directory path, as views into the parsed text.
        struct directory_parts
        {
            std::string_view path;
            std::string_view name;
        };

        //-------------------------------------------------------------------------
        directory_parts split_directory_path(std::string_view full_path)
        {
            const std::size_t last_separator = detail::find_last_separator(full_path);
            if (last_separator == std::string_view::npos)
            {
                // A single segment has no parent.
                return directory_parts{.path = std::string_view(), .name = full_path};
            }

            return directory_parts{.path = full_path.substr(0, last_separator), .name = full_path.substr(last_separator + 1)};
        }
    } // namespace

    //-------------------------------------------------------------------------
    directory_info_view::directory_info_view(std::string_view full_path)
        : m_path()
        , m_name()
    {
        const directory_parts parts = split_directory_path(full_path);
        m_path = parts.path;
        m_name = parts.name;
    }

    //-------------------------------------------------------------------------
    std::string_view directory_info_view::get_path() const
    {
        return m_path;
    }

    //-------------------------------------------------------------------------
    std::string_view directory_info_view::get_name() const
    {
        return m_name;
    }

    //-------------------------------------------------------------------------
    directory_info::directory_info(std::string_view full_path)
        : m_path()
        , m_name()
    {
        const directory_parts parts = split_directory_path(full_path);
        m_path = parts.path;
        m_name = parts.name;
    }

    //-------------------------------------------------------------------------
    std::string_view directory_info::get_path() const
    {
        return m_path;
    }

    //-------------------------------------------------------------------------
    std::string_view directory_info::get_name() const
    {
        return m_name;
    }

    //-------------------------------------------------------------------------
    std::string directory_info::make_full_path() const
    {
        return detail::make_joined_path(m_path, m_name, std::string_view());
    }

    //-------------------------------------------------------------------------
    std::string make_directory_path(std::string_view path, std::string_view name)
    {
        return detail::make_joined_path(path, name, std::string_view());
    }
} // namespace xvfs
