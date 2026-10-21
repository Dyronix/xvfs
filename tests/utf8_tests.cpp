#include "test_check.h"
#include "xvfs/context.h"
#include "xvfs/fileio.h"
#include "xvfs/input_file.h"
#include "xvfs/native_path.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace
{
    using xvfs::error_code;
    using xvfs_test::check;

    // Non-ASCII names written as escaped UTF-8 bytes, so the test does not depend on the
    // compiler's source or execution character set. Adjacent literals stop a hex escape from
    // swallowing the next letter.
    constexpr std::string_view k_street = "Stra\xc3\x9f"
                                          "e";                                                   // Straße (2-byte sequence)
    constexpr std::string_view k_city = "\xe6\x9d\xb1\xe4\xba\xac";                              // Tokyo in kanji (3-byte sequences)
    constexpr std::string_view k_cafe_file = "caf\xc3\xa9 \xe2\x98\x95.txt";                     // café + hot beverage sign
    constexpr std::string_view k_statue = "\xf0\x9f\x97\xbd saves";                              // Statue of Liberty emoji (4-byte sequence)
    constexpr std::string_view k_data_root = "\xd0\xb4\xd0\xb0\xd0\xbd\xd0\xbd\xd1\x8b\xd0\xb5"; // "data" in Cyrillic

    //-------------------------------------------------------------------------
    std::string make_text(const std::u8string& utf8)
    {
        std::string text(utf8.size(), '\0');
        std::ranges::transform(utf8, text.begin(), [](char8_t c) { return static_cast<char>(c); });

        return text;
    }

    //-------------------------------------------------------------------------
    std::string join(std::initializer_list<std::string_view> parts)
    {
        std::string joined;
        for (const std::string_view part : parts)
        {
            if (!joined.empty())
            {
                joined.push_back('/');
            }
            joined.append(part);
        }

        return joined;
    }

    //-------------------------------------------------------------------------
    // A context whose wildcard directory itself has a non-ASCII name.
    xvfs::context make_context(const xvfs_test::temp_directory& directory)
    {
        xvfs::context vfs;
        const std::string root = xvfs::make_utf8_path(directory.get_path()) + "/" + std::string(k_data_root);
        const error_code added = vfs.add_wildcard("test:", root);
        static_cast<void>(added);

        return vfs;
    }

    //-------------------------------------------------------------------------
    check well_formed_utf8_is_accepted_and_malformed_utf8_is_rejected()
    {
        check c;
        for (const std::string_view valid : {std::string_view(), std::string_view("plain/ascii.txt"), k_street, k_city, k_cafe_file, k_statue, std::string_view("\xf4\x8f\xbf\xbf")})
        {
            c.expect(xvfs::is_valid_utf8(valid), "valid");
        }

        // Stray continuation, invalid byte, overlong '/', overlong 3-byte, surrogate,
        // truncated, above U+10FFFF, bad continuation, truncated at the end of the text.
        const std::array invalid = {
            std::string_view("\x80"),
            std::string_view("\xff"),
            std::string_view("\xc0\xaf"),
            std::string_view("\xe0\x80\xaf"),
            std::string_view("\xed\xa0\x80"),
            std::string_view("\xe6\x9d"),
            std::string_view("\xf4\x90\x80\x80"),
            std::string_view("\xc3\x28"),
            std::string_view("ok\xf0\x9f\x97"),
        };
        for (const std::string_view text : invalid)
        {
            c.expect(!xvfs::is_valid_utf8(text), "invalid");
            c.expect(xvfs::make_native_path(text).get_error() == error_code::invalid_utf8, "make_native_path rejects it");
        }

        return c;
    }

    //-------------------------------------------------------------------------
    check native_paths_carry_the_utf8_text_both_ways()
    {
        check c;
        const std::string text = join({k_data_root, k_street, k_city, k_cafe_file});
        const xvfs::result<std::filesystem::path> native = xvfs::make_native_path(text);
        c.expect(native.has_value(), "converted");
        if (!native.has_value())
        {
            return c;
        }

        c.expect(make_text(native.get_value().u8string()) == text, "the path holds the same code points");
        c.expect(make_text(native.get_value().filename().u8string()) == k_cafe_file, "the file name survives");
        c.expect(xvfs::make_utf8_path(native.get_value()) == text, "and converts back");

        const std::filesystem::path built = std::filesystem::path(u8"a") / std::filesystem::path(u8"b");
        c.expect(xvfs::make_utf8_path(built) == "a/b", "forward slashes");

        return c;
    }

    //-------------------------------------------------------------------------
    check files_with_non_ascii_names_are_read_and_found()
    {
        check c;
        const xvfs_test::temp_directory directory;
        const std::string relative = join({k_data_root, k_street, k_city, k_cafe_file});
        directory.write_file(relative, "espresso");
        const xvfs::context vfs = make_context(directory);

        const std::string content_path = "test:/" + join({k_street, k_city, k_cafe_file});
        const xvfs::result<std::string> text = xvfs::read_text_file(vfs, content_path);
        c.expect(text.has_value() && text.get_value() == "espresso", "read_text_file");

        const xvfs::result<std::vector<std::byte>> bytes = xvfs::read_binary_file(vfs, content_path);
        c.expect(bytes.has_value() && bytes.get_value().size() == std::string_view("espresso").size(), "read_binary_file");

        c.expect(xvfs::is_file(vfs, content_path), "is_file");
        c.expect(xvfs::is_directory(vfs, "test:/" + join({k_street, k_city})), "is_directory");
        c.expect(!xvfs::is_file(vfs, "test:/" + join({k_street, k_cafe_file})), "a different directory does not have it");

        xvfs::result<xvfs::input_file> opened = xvfs::open_input_file(vfs, content_path);
        c.expect(opened.has_value(), "open_input_file");
        if (opened.has_value())
        {
            const xvfs::input_file file = std::move(opened).get_value();
            c.expect(file.get_path() == vfs.resolve_path(content_path).get_value(), "get_path is the UTF-8 resolved path");
        }

        return c;
    }

    //-------------------------------------------------------------------------
    check created_directories_carry_the_utf8_name_on_disk()
    {
        check c;
        const xvfs_test::temp_directory directory;
        const xvfs::context vfs = make_context(directory);

        c.expect(xvfs::create_directories(vfs, "test:/" + join({k_statue, k_city})) == error_code::none, "created");
        c.expect(xvfs::is_directory(vfs, "test:/" + join({k_statue, k_city})), "found again");

        // Ask the standard library, not xvfs, what the names on disk are.
        std::vector<std::string> names;
        std::error_code error;
        for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(directory.get_path(), error))
        {
            names.push_back(make_text(entry.path().filename().u8string()));
        }
        c.expect(!error, "listed");

        const auto has_name = [&names](std::string_view name) { return std::ranges::find(names, name) != names.end(); };
        c.expect(has_name(k_data_root), "the wildcard directory");
        c.expect(has_name(k_statue), "the emoji directory");
        c.expect(has_name(k_city), "the kanji directory");

        return c;
    }

    //-------------------------------------------------------------------------
    check malformed_utf8_paths_fail_with_an_error()
    {
        check c;
        const xvfs_test::temp_directory directory;
        std::vector<std::string> errors;
        xvfs::context vfs = make_context(directory);
        vfs.set_log_callback([&errors](xvfs::log_level level, std::string_view message) {
            if (level == xvfs::log_level::error)
            {
                errors.emplace_back(message);
            }
        });

        const std::string bad = "test:/caf\xc3.txt";
        // Two reads, two queries and one create, each logged once.
        constexpr std::size_t k_failing_calls = 5;
        c.expect(vfs.resolve_path(bad).has_value(), "resolving is text only and passes the bytes through");
        c.expect(xvfs::read_text_file(vfs, bad).get_error() == error_code::invalid_utf8, "read_text_file");
        c.expect(xvfs::read_binary_file(vfs, bad).get_error() == error_code::invalid_utf8, "read_binary_file");
        c.expect(!xvfs::is_file(vfs, bad) && !xvfs::is_directory(vfs, bad), "queries are false");
        c.expect(xvfs::create_directories(vfs, bad) == error_code::invalid_utf8, "create_directories");
        c.expect(errors.size() == k_failing_calls, "each failure is logged");

        return c;
    }

    constexpr std::array k_tests = {
        xvfs_test::test_case{.name = "well_formed_utf8_is_accepted_and_malformed_utf8_is_rejected", .run = well_formed_utf8_is_accepted_and_malformed_utf8_is_rejected},
        xvfs_test::test_case{.name = "native_paths_carry_the_utf8_text_both_ways", .run = native_paths_carry_the_utf8_text_both_ways},
        xvfs_test::test_case{.name = "files_with_non_ascii_names_are_read_and_found", .run = files_with_non_ascii_names_are_read_and_found},
        xvfs_test::test_case{.name = "created_directories_carry_the_utf8_name_on_disk", .run = created_directories_carry_the_utf8_name_on_disk},
        xvfs_test::test_case{.name = "malformed_utf8_paths_fail_with_an_error", .run = malformed_utf8_paths_fail_with_an_error},
    };
} // namespace

namespace xvfs_test
{
    //-------------------------------------------------------------------------
    std::span<const test_case> get_utf8_tests()
    {
        return k_tests;
    }
} // namespace xvfs_test
