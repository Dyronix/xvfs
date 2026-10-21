#include "xvfs/file_info.h"

#include "path_text.h"

#include <cstddef>

namespace xvfs
{
    namespace
    {
        // The three parts of a file path, as views into the parsed text.
        struct file_parts
        {
            std::string_view path;
            std::string_view filename;
            std::string_view extension;
        };

        //-------------------------------------------------------------------------
        file_parts split_file_path(std::string_view full_path)
        {
            const std::size_t last_separator = detail::find_last_separator(full_path);
            const bool has_separator = last_separator != std::string_view::npos;
            const std::size_t segment_start = has_separator ? last_separator + 1 : 0;

            // Only the last segment can hold the extension, so "a.v2/file" has none.
            const std::string_view segment = full_path.substr(segment_start);
            const std::size_t dot = segment.find_last_of('.');

            file_parts parts;
            parts.path = has_separator ? full_path.substr(0, last_separator) : std::string_view();
            parts.filename = segment.substr(0, dot);
            parts.extension = dot == std::string_view::npos ? std::string_view() : segment.substr(dot);

            return parts;
        }
    } // namespace

    //-------------------------------------------------------------------------
    file_info_view::file_info_view(std::string_view full_path)
        : m_path()
        , m_filename()
        , m_extension()
    {
        const file_parts parts = split_file_path(full_path);
        m_path = parts.path;
        m_filename = parts.filename;
        m_extension = parts.extension;
    }

    //-------------------------------------------------------------------------
    std::string_view file_info_view::get_path() const
    {
        return m_path;
    }

    //-------------------------------------------------------------------------
    std::string_view file_info_view::get_filename() const
    {
        return m_filename;
    }

    //-------------------------------------------------------------------------
    std::string_view file_info_view::get_extension() const
    {
        return m_extension;
    }

    //-------------------------------------------------------------------------
    file_info::file_info(std::string_view full_path)
        : m_path()
        , m_filename()
        , m_extension()
    {
        const file_parts parts = split_file_path(full_path);
        m_path = parts.path;
        m_filename = parts.filename;
        m_extension = parts.extension;
    }

    //-------------------------------------------------------------------------
    std::string_view file_info::get_path() const
    {
        return m_path;
    }

    //-------------------------------------------------------------------------
    std::string_view file_info::get_filename() const
    {
        return m_filename;
    }

    //-------------------------------------------------------------------------
    std::string_view file_info::get_extension() const
    {
        return m_extension;
    }

    //-------------------------------------------------------------------------
    std::string file_info::make_full_path() const
    {
        return detail::make_joined_path(m_path, m_filename, m_extension);
    }

    //-------------------------------------------------------------------------
    std::string make_file_path(std::string_view path, std::string_view filename, std::string_view extension)
    {
        return detail::make_joined_path(path, filename, extension);
    }
} // namespace xvfs
