#pragma once

#include "xvfs/log.h"

#include <initializer_list>
#include <string_view>

// Internal to xvfs (not installed): builds a log line from parts and hands it to the callback.
// Nothing is built or allocated when the callback is empty.
// Threading: safe from any thread when the callback is.

namespace xvfs::detail
{
    void log_message(const log_callback& callback, log_level level, std::initializer_list<std::string_view> parts);
} // namespace xvfs::detail
