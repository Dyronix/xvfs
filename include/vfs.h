#pragma once

#include <string>

namespace vfs
{
    void add_wildcard(const std::string& wildcard, const std::string& value);

    std::string resolve_path(const std::string& filename);

    bool exists_filepath(const std::string& filename);
}