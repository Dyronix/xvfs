#include "xvfs/error.h"

namespace xvfs
{
    //-------------------------------------------------------------------------
    std::string_view get_error_message(error_code error)
    {
        switch (error)
        {
        case error_code::none:
            return "no error";
        case error_code::empty_path:
            return "the path is empty";
        case error_code::invalid_utf8:
            return "the path is not well-formed UTF-8";
        case error_code::empty_wildcard:
            return "the wildcard is empty";
        case error_code::no_wildcards:
            return "no wildcards are registered, so the path cannot be resolved";
        case error_code::buffer_too_small:
            return "the buffer is too small for the resolved path";
        case error_code::not_a_file:
            return "the path does not name a regular file";
        case error_code::open_failed:
            return "the file could not be opened";
        case error_code::size_query_failed:
            return "the size of the file could not be read";
        case error_code::read_failed:
            return "the file could not be read completely";
        case error_code::filesystem_error:
            return "the filesystem operation failed";
        }

        return "unknown error";
    }
} // namespace xvfs
