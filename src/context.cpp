#include "xvfs/context.h"

#include "log_message.h"
#include "path_text.h"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace xvfs
{
    namespace
    {
        // The pieces a resolved path is written from, so both resolve functions share one rule.
        struct resolve_plan
        {
            std::string_view directory;
            std::string_view remainder;
            bool needs_separator = false;
            std::size_t size = 0;
        };

        //-------------------------------------------------------------------------
        std::string make_normalized_copy(std::string_view text)
        {
            std::string copy(text);
            std::ranges::transform(copy, copy.begin(), detail::to_forward_slash);

            return copy;
        }

        //-------------------------------------------------------------------------
        bool starts_with_normalized(std::string_view path, std::string_view prefix)
        {
            if (path.size() < prefix.size())
            {
                return false;
            }

            const std::string_view head = path.substr(0, prefix.size());

            return std::ranges::equal(head, prefix, [](char a, char b) { return detail::to_forward_slash(a) == b; });
        }

        //-------------------------------------------------------------------------
        std::string_view strip_leading_separators(std::string_view text)
        {
            const std::size_t first = text.find_first_not_of(detail::k_separators);

            return first == std::string_view::npos ? std::string_view() : text.substr(first);
        }

        //-------------------------------------------------------------------------
        resolve_plan make_plan(std::string_view directory, std::string_view remainder)
        {
            const std::string_view rest = strip_leading_separators(remainder);

            resolve_plan plan;
            if (rest.empty())
            {
                plan.directory = directory;
            }
            else if (directory.empty())
            {
                plan.remainder = rest;
            }
            else
            {
                plan.directory = directory;
                plan.remainder = rest;
                plan.needs_separator = directory.back() != detail::k_separator;
            }

            const std::size_t separator_size = plan.needs_separator ? 1 : 0;
            plan.size = plan.directory.size() + separator_size + plan.remainder.size();

            return plan;
        }

        //-------------------------------------------------------------------------
        std::string_view write_plan(const resolve_plan& plan, std::span<char> buffer)
        {
            // The directory was normalized when it was added; the remainder is caller text.
            auto out = std::ranges::copy(plan.directory, buffer.begin()).out;
            if (plan.needs_separator)
            {
                *out = detail::k_separator;
                ++out;
            }
            std::ranges::transform(plan.remainder, out, detail::to_forward_slash);

            return std::string_view(buffer.data(), plan.size);
        }
    } // namespace

    //-------------------------------------------------------------------------
    context::context()
        : m_wildcards()
        , m_log_callback()
    {
    }

    //-------------------------------------------------------------------------
    void context::set_log_callback(log_callback callback)
    {
        m_log_callback = std::move(callback);
    }

    //-------------------------------------------------------------------------
    const log_callback& context::get_log_callback() const
    {
        return m_log_callback;
    }

    //-------------------------------------------------------------------------
    error_code context::add_wildcard(std::string_view wildcard, std::string_view directory)
    {
        if (wildcard.empty())
        {
            detail::log_message(m_log_callback, log_level::error, {"[xvfs] Cannot add an empty wildcard for directory '", directory, "'"});
            return error_code::empty_wildcard;
        }

        std::string name = make_normalized_copy(wildcard);
        std::string real_directory = make_normalized_copy(directory);

        detail::log_message(m_log_callback, log_level::info, {"[xvfs] Adding wildcard '", name, "' for directory '", real_directory, "'"});

        const auto existing = std::ranges::find(m_wildcards, name, &wildcard_entry::name);
        if (existing != m_wildcards.end())
        {
            existing->directory = std::move(real_directory);
            return error_code::none;
        }

        m_wildcards.push_back(wildcard_entry{.name = std::move(name), .directory = std::move(real_directory)});

        return error_code::none;
    }

    //-------------------------------------------------------------------------
    result<std::string> context::resolve_path(std::string_view path) const
    {
        const error_code error = check_resolvable(path);
        if (error != error_code::none)
        {
            return result<std::string>(error);
        }

        const wildcard_entry* match = find_longest_match(path);
        if (match == nullptr)
        {
            return result<std::string>(make_normalized_copy(path));
        }

        const resolve_plan plan = make_plan(match->directory, path.substr(match->name.size()));

        std::string resolved(plan.size, '\0');
        const std::string_view written = write_plan(plan, std::span<char>(resolved));
        resolved.resize(written.size());

        return result<std::string>(std::move(resolved));
    }

    //-------------------------------------------------------------------------
    result<std::string_view> context::resolve_path_into(std::string_view path, std::span<char> buffer) const
    {
        const error_code error = check_resolvable(path);
        if (error != error_code::none)
        {
            return result<std::string_view>(error);
        }

        const wildcard_entry* match = find_longest_match(path);
        const resolve_plan plan = match == nullptr ? resolve_plan{.directory = std::string_view(), .remainder = path, .needs_separator = false, .size = path.size()} : make_plan(match->directory, path.substr(match->name.size()));

        if (plan.size > buffer.size())
        {
            detail::log_message(m_log_callback, log_level::error, {"[xvfs] The buffer is too small to resolve '", path, "'"});
            return result<std::string_view>(error_code::buffer_too_small);
        }

        return result<std::string_view>(write_plan(plan, buffer));
    }

    //-------------------------------------------------------------------------
    const context::wildcard_entry* context::find_longest_match(std::string_view path) const
    {
        const wildcard_entry* longest = nullptr;
        for (const wildcard_entry& candidate : m_wildcards)
        {
            const bool is_longer = longest == nullptr || candidate.name.size() > longest->name.size();
            if (is_longer && starts_with_normalized(path, candidate.name))
            {
                longest = &candidate;
            }
        }

        return longest;
    }

    //-------------------------------------------------------------------------
    error_code context::check_resolvable(std::string_view path) const
    {
        if (path.empty())
        {
            detail::log_message(m_log_callback, log_level::error, {"[xvfs] Cannot resolve an empty path"});
            return error_code::empty_path;
        }

        if (m_wildcards.empty())
        {
            detail::log_message(m_log_callback, log_level::warning, {"[xvfs] No wildcards are registered to resolve '", path, "'"});
            return error_code::no_wildcards;
        }

        return error_code::none;
    }
} // namespace xvfs
