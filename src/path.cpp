#include "xvfs/path.h"

#include "path_text.h"

#include <algorithm>
#include <cstddef>

namespace xvfs
{
    //-------------------------------------------------------------------------
    std::string make_normalized_path(std::string_view path)
    {
        std::string normalized(path);
        std::ranges::transform(normalized, normalized.begin(), detail::to_forward_slash);

        return normalized;
    }

    //-------------------------------------------------------------------------
    std::string detail::make_joined_path(std::string_view directory, std::string_view name, std::string_view extension)
    {
        const bool needs_separator = !directory.empty() && !is_separator(directory.back());
        const std::size_t separator_size = needs_separator ? 1 : 0;

        std::string joined;
        joined.reserve(directory.size() + separator_size + name.size() + extension.size());
        joined.append(directory);
        if (needs_separator)
        {
            joined.push_back(k_separator);
        }
        joined.append(name);
        joined.append(extension);

        std::ranges::transform(joined, joined.begin(), to_forward_slash);

        return joined;
    }
} // namespace xvfs
