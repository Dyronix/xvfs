#include "log_message.h"

#include <cstddef>
#include <string>

namespace xvfs::detail
{
    //-------------------------------------------------------------------------
    void log_message(const log_callback& callback, log_level level, std::initializer_list<std::string_view> parts)
    {
        if (!callback)
        {
            return;
        }

        std::size_t size = 0;
        for (const std::string_view part : parts)
        {
            size += part.size();
        }

        std::string message;
        message.reserve(size);
        for (const std::string_view part : parts)
        {
            message.append(part);
        }

        callback(level, message);
    }
} // namespace xvfs::detail
