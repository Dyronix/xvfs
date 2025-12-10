#pragma once

#include "string/string_ops.h"

#include <string>
#include <sstream>
#include <cassert>

namespace vfs
{
	namespace internal
	{
		char get_separator();

		std::string ensure_trailing_separator(std::string_view path);
	}

    //======================================================================
    // FILE PARSING
    //======================================================================
    struct parsed_path
    {
        parsed_path(std::string_view full_path)
        {
            char sep = internal::get_separator();

            auto last_sep = full_path.find_last_of(sep);
            auto dot_pos  = full_path.find_last_of('.');

            assert(dot_pos != std::string::npos && "file_info_view must have a filename and an extension");

            path = (last_sep != std::string_view::npos) ? full_path.substr(0, last_sep) : "";
            filename = (last_sep != std::string_view::npos)
                ? full_path.substr(last_sep + 1, dot_pos - last_sep - 1)
                : full_path.substr(0, dot_pos);
            extension = full_path.substr(dot_pos);
        }

        std::string_view path;
        std::string_view filename;
        std::string_view extension;
    };

    //======================================================================
    // DIRECTORY PARSING
    //======================================================================
    struct parsed_directory
    {
        parsed_directory(std::string_view full_path)
        {
            char sep = internal::get_separator();
            auto last_sep = full_path.find_last_of(sep);

            if (last_sep == std::string_view::npos)
            {
                // Only a single segment, no parent path
                path = "";
                name = full_path;
            }
            else
            {
                path = full_path.substr(0, last_sep);
                name = full_path.substr(last_sep + 1);
            }
        }

        std::string_view path; // parent path (may be empty)
        std::string_view name; // last directory segment
    };

    //======================================================================
    // FILE INFO VIEW
    //======================================================================
    class file_info_view
    {
    public:
        file_info_view(std::string_view full_path)
		{
			parse_full_path(full_path);
		}

        std::string_view path() const      { return m_path; }
        std::string_view filename() const  { return m_filename; }
        std::string_view extension() const { return m_extension; }

    private:
        void parse_full_path(std::string_view full_path)
		{
			parsed_path parsed_path(full_path);

			m_path		= parsed_path.path;
			m_filename	= parsed_path.filename;
			m_extension = parsed_path.extension;
		}

    private:
        std::string_view m_path;
        std::string_view m_filename;
        std::string_view m_extension;
    };

    //======================================================================
    // DIRECTORY INFO VIEW
    //======================================================================
    class directory_info_view
    {
    public:
        directory_info_view(std::string_view full_path)
        {
            parse_full_path(full_path);
        }

        std::string_view path() const { return m_path; }      // parent path
        std::string_view name() const { return m_name; }      // directory name

    private:
        void parse_full_path(std::string_view full_path)
        {
            parsed_directory parsed(full_path);
            m_path = parsed.path;
            m_name = parsed.name;
        }

    private:
        std::string_view m_path;
        std::string_view m_name;
    };

    //======================================================================
    // FILE INFO (OWNING)
    //======================================================================
    class file_info
    {
    public:
        //-------------------------------------------------------------------------
        static std::string make_full_path(std::string_view full_path)
        {
            parsed_path parsed_path(full_path);

            auto path      = parsed_path.path;
            auto filename  = parsed_path.filename;
            auto extension = parsed_path.extension;

            std::stringstream stream;

            stream << internal::ensure_trailing_separator(path);
            stream << filename;
            stream << extension;

            return string::string_replace<std::string>(stream.str(), "\\", "/");
        }

        //-------------------------------------------------------------------------
        static std::string make_full_path(std::string_view path, std::string_view filename, std::string_view extension)
        {
            std::stringstream stream;

            stream << internal::ensure_trailing_separator(path);
            stream << filename;
            stream << extension;

            return string::string_replace<std::string>(stream.str(), "\\", "/");
        }

    public:
        //-------------------------------------------------------------------------
        file_info(std::string_view full_path)
        {
            parse_full_path(full_path);
        }

        //-------------------------------------------------------------------------
        std::string full_path() const
        {
            std::string full_path = internal::ensure_trailing_separator(m_path) + m_filename + m_extension;

            return string::string_replace<std::string>(full_path, "\\", "/");
        }

    private:
        void parse_full_path(std::string_view full_path)
		{
			parsed_path parsed_path(full_path);

			m_path		= parsed_path.path;
			m_filename	= parsed_path.filename;
			m_extension = parsed_path.extension;
		}

    private:
        std::string m_path;
        std::string m_filename;
        std::string m_extension;
    };

    //======================================================================
    // DIRECTORY INFO (OWNING)
    //======================================================================
    class directory_info
    {
    public:
        //-------------------------------------------------------------------------
        static std::string make_full_path(std::string_view full_path)
        {
            parsed_directory parsed(full_path);

            std::stringstream stream;
            stream << internal::ensure_trailing_separator(parsed.path);
            stream << parsed.name;

            return string::string_replace<std::string>(stream.str(), "\\", "/");
        }

        //-------------------------------------------------------------------------
        static std::string make_full_path(std::string_view path, std::string_view name)
        {
            std::stringstream stream;

            stream << internal::ensure_trailing_separator(path);
            stream << name;

            return string::string_replace<std::string>(stream.str(), "\\", "/");
        }

    public:
        directory_info(std::string_view full_path)
        {
            parse_full_path(full_path);
        }

        std::string full_path() const
        {
            std::string full_path = internal::ensure_trailing_separator(m_path) + m_name;
            return string::string_replace<std::string>(full_path, "\\", "/");
        }

    private:
        void parse_full_path(std::string_view full_path)
        {
            parsed_directory parsed(full_path);
            m_path = std::string(parsed.path);
            m_name = std::string(parsed.name);
        }

    private:
        std::string m_path;
        std::string m_name;
    };
}