#include "test_check.h"
#include "xvfs/context.h"
#include "xvfs/error.h"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    using xvfs::error_code;
    using xvfs_test::check;

    // Large enough for every path in these tests.
    constexpr std::size_t k_buffer_size = 256;

    //-------------------------------------------------------------------------
    xvfs::context make_local_context(std::string_view directory)
    {
        xvfs::context vfs;
        const error_code added = vfs.add_wildcard("local:", directory);
        static_cast<void>(added);

        return vfs;
    }

    //-------------------------------------------------------------------------
    std::string resolve_or_empty(const xvfs::context& vfs, std::string_view path)
    {
        xvfs::result<std::string> resolved = vfs.resolve_path(path);

        return resolved.has_value() ? std::move(resolved).get_value() : std::string();
    }

    //-------------------------------------------------------------------------
    check resolving_without_wildcards_is_an_error()
    {
        check c;
        const xvfs::context vfs;
        const xvfs::result<std::string> resolved = vfs.resolve_path("local:/a.txt");
        c.expect(!resolved.has_value(), "no value");
        c.expect(resolved.get_error() == error_code::no_wildcards, "no_wildcards");

        return c;
    }

    //-------------------------------------------------------------------------
    check resolving_an_empty_path_is_an_error()
    {
        check c;
        const xvfs::context vfs = make_local_context("/root");
        c.expect(vfs.resolve_path("").get_error() == error_code::empty_path, "empty_path");

        return c;
    }

    //-------------------------------------------------------------------------
    check an_empty_wildcard_is_rejected()
    {
        check c;
        xvfs::context vfs;
        c.expect(vfs.add_wildcard("", "/root") == error_code::empty_wildcard, "empty_wildcard");
        c.expect(vfs.resolve_path("a.txt").get_error() == error_code::no_wildcards, "nothing was added");

        return c;
    }

    //-------------------------------------------------------------------------
    check a_wildcard_prefix_becomes_its_directory()
    {
        check c;
        const xvfs::context vfs = make_local_context("/root");
        c.expect(resolve_or_empty(vfs, "local:/content/shaders/lit.vs") == "/root/content/shaders/lit.vs", "file");
        c.expect(resolve_or_empty(vfs, "local:/content/shaders") == "/root/content/shaders", "directory");
        c.expect(resolve_or_empty(vfs, "local:/content/") == "/root/content/", "trailing separator kept");
        c.expect(resolve_or_empty(vfs, "local:") == "/root", "the wildcard alone");
        c.expect(resolve_or_empty(vfs, "local:file.txt") == "/root/file.txt", "no separator after the wildcard");

        return c;
    }

    //-------------------------------------------------------------------------
    check a_directory_with_a_trailing_separator_gives_one_separator()
    {
        check c;
        const xvfs::context vfs = make_local_context("/root/");
        c.expect(resolve_or_empty(vfs, "local:/a.txt") == "/root/a.txt", "one separator");
        c.expect(resolve_or_empty(vfs, "local://a.txt") == "/root/a.txt", "repeated separators after the wildcard collapse");

        const xvfs::context root = make_local_context("/");
        c.expect(resolve_or_empty(root, "local:/a.txt") == "/a.txt", "filesystem root");

        return c;
    }

    //-------------------------------------------------------------------------
    check an_empty_directory_gives_a_relative_path()
    {
        check c;
        const xvfs::context vfs = make_local_context("");
        c.expect(resolve_or_empty(vfs, "local:/content/a.txt") == "content/a.txt", "relative");

        return c;
    }

    //-------------------------------------------------------------------------
    check the_longest_matching_wildcard_wins_in_any_order()
    {
        check c;
        xvfs::context first;
        c.expect(first.add_wildcard("content:", "/base") == error_code::none, "add");
        c.expect(first.add_wildcard("content:dlc", "/dlc") == error_code::none, "add");

        xvfs::context second;
        c.expect(second.add_wildcard("content:dlc", "/dlc") == error_code::none, "add");
        c.expect(second.add_wildcard("content:", "/base") == error_code::none, "add");

        for (const xvfs::context* vfs : {&first, &second})
        {
            c.expect(resolve_or_empty(*vfs, "content:dlc/map.bin") == "/dlc/map.bin", "longest");
            c.expect(resolve_or_empty(*vfs, "content:/map.bin") == "/base/map.bin", "shorter");
        }

        return c;
    }

    //-------------------------------------------------------------------------
    check a_wildcard_only_matches_at_the_start()
    {
        check c;
        const xvfs::context vfs = make_local_context("/root");
        c.expect(resolve_or_empty(vfs, "data/local:/a.txt") == "data/local:/a.txt", "not in the middle");
        c.expect(resolve_or_empty(vfs, "/abs/path/a.txt") == "/abs/path/a.txt", "absolute paths pass through");

        return c;
    }

    //-------------------------------------------------------------------------
    check backslashes_become_forward_slashes()
    {
        check c;
        const xvfs::context vfs = make_local_context("C:\\game\\content");
        c.expect(resolve_or_empty(vfs, "local:\\maps\\harbor.map") == "C:/game/content/maps/harbor.map", "matched");
        c.expect(resolve_or_empty(vfs, "D:\\other\\a.txt") == "D:/other/a.txt", "passed through");

        xvfs::context slash_wildcard;
        c.expect(slash_wildcard.add_wildcard("mods\\", "/mods") == error_code::none, "add");
        c.expect(resolve_or_empty(slash_wildcard, "mods/a.txt") == "/mods/a.txt", "wildcard separators are normalized too");

        return c;
    }

    //-------------------------------------------------------------------------
    check adding_a_wildcard_again_replaces_its_directory()
    {
        check c;
        xvfs::context vfs = make_local_context("/old");
        c.expect(vfs.add_wildcard("local:", "/new") == error_code::none, "replace");
        c.expect(resolve_or_empty(vfs, "local:/a.txt") == "/new/a.txt", "the replaced directory");

        return c;
    }

    //-------------------------------------------------------------------------
    check resolving_into_a_buffer_writes_the_same_path()
    {
        check c;
        const xvfs::context vfs = make_local_context("/root");
        std::array<char, k_buffer_size> buffer{};

        for (const std::string_view path : {"local:/a/b.txt", "local:", "/abs/c.txt", "local:\\x\\y"})
        {
            const xvfs::result<std::string_view> written = vfs.resolve_path_into(path, buffer);
            c.expect(written.has_value(), "has a value");
            c.expect(written.get_value() == resolve_or_empty(vfs, path), "same as resolve_path");
            c.expect(written.get_value().data() == buffer.data(), "views the buffer");
        }

        return c;
    }

    //-------------------------------------------------------------------------
    check a_small_buffer_is_an_error_and_stays_untouched()
    {
        check c;
        const xvfs::context vfs = make_local_context("/root");
        const std::string_view path = "local:/a.txt";
        const std::size_t needed = std::string_view("/root/a.txt").size();

        std::vector<char> small(needed - 1, '#');
        const xvfs::result<std::string_view> failed = vfs.resolve_path_into(path, small);
        c.expect(failed.get_error() == error_code::buffer_too_small, "buffer_too_small");
        c.expect(std::string_view(small.data(), small.size()) == std::string(needed - 1, '#'), "untouched");

        std::vector<char> exact(needed);
        const xvfs::result<std::string_view> fits = vfs.resolve_path_into(path, exact);
        c.expect(fits.has_value() && fits.get_value() == "/root/a.txt", "an exact buffer fits");

        return c;
    }

    //-------------------------------------------------------------------------
    check the_log_callback_receives_messages()
    {
        check c;
        std::vector<xvfs::log_level> levels;
        std::vector<std::string> messages;

        xvfs::context vfs;
        vfs.set_log_callback([&](xvfs::log_level level, std::string_view message) {
            levels.push_back(level);
            messages.emplace_back(message);
        });

        const xvfs::result<std::string> early = vfs.resolve_path("local:/a.txt");
        c.expect(!early.has_value(), "fails before any wildcard");
        c.expect(vfs.add_wildcard("local:", "/root") == error_code::none, "add");

        c.expect(levels.size() == 2, "two messages");
        c.expect(levels.size() == 2 && levels[0] == xvfs::log_level::warning, "warning first");
        c.expect(levels.size() == 2 && levels[1] == xvfs::log_level::info, "then info");
        c.expect(messages.size() == 2 && messages[1].find("local:") != std::string::npos, "names the wildcard");

        const std::size_t before = messages.size();
        const xvfs::result<std::string> resolved = vfs.resolve_path("local:/a.txt");
        c.expect(resolved.has_value() && messages.size() == before, "a successful resolve does not log");

        vfs.set_log_callback(xvfs::log_callback());
        c.expect(vfs.add_wildcard("", "x") == error_code::empty_wildcard && messages.size() == before, "an empty callback is silent");

        return c;
    }

    //-------------------------------------------------------------------------
    check every_error_has_a_message()
    {
        check c;
        for (const error_code error : {error_code::none, error_code::empty_path, error_code::empty_wildcard, error_code::no_wildcards, error_code::buffer_too_small, error_code::not_a_file, error_code::open_failed, error_code::size_query_failed, error_code::read_failed, error_code::filesystem_error})
        {
            const std::string_view message = xvfs::get_error_message(error);
            c.expect(!message.empty() && message != "unknown error", "message");
        }

        return c;
    }

    //-------------------------------------------------------------------------
    check a_copied_context_is_independent()
    {
        check c;
        const xvfs::context original = make_local_context("/root");
        xvfs::context copy = original;
        c.expect(copy.add_wildcard("local:", "/other") == error_code::none, "add");
        c.expect(resolve_or_empty(original, "local:/a") == "/root/a", "original unchanged");
        c.expect(resolve_or_empty(copy, "local:/a") == "/other/a", "copy changed");

        return c;
    }

    constexpr std::array k_tests = {
        xvfs_test::test_case{.name = "resolving_without_wildcards_is_an_error", .run = resolving_without_wildcards_is_an_error},
        xvfs_test::test_case{.name = "resolving_an_empty_path_is_an_error", .run = resolving_an_empty_path_is_an_error},
        xvfs_test::test_case{.name = "an_empty_wildcard_is_rejected", .run = an_empty_wildcard_is_rejected},
        xvfs_test::test_case{.name = "a_wildcard_prefix_becomes_its_directory", .run = a_wildcard_prefix_becomes_its_directory},
        xvfs_test::test_case{.name = "a_directory_with_a_trailing_separator_gives_one_separator", .run = a_directory_with_a_trailing_separator_gives_one_separator},
        xvfs_test::test_case{.name = "an_empty_directory_gives_a_relative_path", .run = an_empty_directory_gives_a_relative_path},
        xvfs_test::test_case{.name = "the_longest_matching_wildcard_wins_in_any_order", .run = the_longest_matching_wildcard_wins_in_any_order},
        xvfs_test::test_case{.name = "a_wildcard_only_matches_at_the_start", .run = a_wildcard_only_matches_at_the_start},
        xvfs_test::test_case{.name = "backslashes_become_forward_slashes", .run = backslashes_become_forward_slashes},
        xvfs_test::test_case{.name = "adding_a_wildcard_again_replaces_its_directory", .run = adding_a_wildcard_again_replaces_its_directory},
        xvfs_test::test_case{.name = "resolving_into_a_buffer_writes_the_same_path", .run = resolving_into_a_buffer_writes_the_same_path},
        xvfs_test::test_case{.name = "a_small_buffer_is_an_error_and_stays_untouched", .run = a_small_buffer_is_an_error_and_stays_untouched},
        xvfs_test::test_case{.name = "the_log_callback_receives_messages", .run = the_log_callback_receives_messages},
        xvfs_test::test_case{.name = "every_error_has_a_message", .run = every_error_has_a_message},
        xvfs_test::test_case{.name = "a_copied_context_is_independent", .run = a_copied_context_is_independent},
    };
} // namespace

namespace xvfs_test
{
    //-------------------------------------------------------------------------
    std::span<const test_case> get_context_tests()
    {
        return k_tests;
    }
} // namespace xvfs_test
