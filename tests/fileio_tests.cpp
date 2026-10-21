#include "test_check.h"
#include "xvfs/context.h"
#include "xvfs/fileio.h"
#include "xvfs/input_file.h"
#include "xvfs/native_path.h"

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

    // Bytes that a text read would mangle if it converted newlines or stopped at a zero.
    constexpr std::string_view k_binary_contents{"a\0b\r\nc\xff", 7};

    // A file whose bytes are easy to check one by one.
    constexpr std::string_view k_letters = "abcdef";

    // Shorter and longer than k_letters, for partial and failing reads.
    constexpr std::size_t k_short_read = 3;
    constexpr std::size_t k_long_read = 8;

    // Counts the allocations it serves, to show the caller's allocator is used.
    // The counter is not owned; the test that makes the allocator keeps it alive.
    template <typename T>
    class counting_allocator
    {
    public:
        using value_type = T;

        explicit counting_allocator(int32_t* count)
            : m_count(count)
        {
        }

        template <typename U>
        explicit counting_allocator(const counting_allocator<U>& other)
            : m_count(other.get_count_pointer())
        {
        }

        T* allocate(std::size_t n)
        {
            ++*m_count;
            return std::allocator<T>().allocate(n);
        }

        void deallocate(T* p, std::size_t n)
        {
            std::allocator<T>().deallocate(p, n);
        }

        int32_t* get_count_pointer() const
        {
            return m_count;
        }

        bool operator==(const counting_allocator& other) const
        {
            return m_count == other.m_count;
        }

    private:
        int32_t* m_count;
    };

    //-------------------------------------------------------------------------
    xvfs::context make_context(const xvfs_test::temp_directory& directory)
    {
        xvfs::context vfs;
        const error_code added = vfs.add_wildcard("test:", xvfs::make_utf8_path(directory.get_path()));
        static_cast<void>(added);

        return vfs;
    }

    //-------------------------------------------------------------------------
    check a_binary_file_is_read_byte_for_byte()
    {
        check c;
        const xvfs_test::temp_directory directory;
        directory.write_file("data/blob.bin", k_binary_contents);
        const xvfs::context vfs = make_context(directory);

        const xvfs::result<std::vector<std::byte>> bytes = xvfs::read_binary_file(vfs, "test:/data/blob.bin");
        c.expect(bytes.has_value(), "read");
        c.expect(bytes.has_value() && bytes.get_value().size() == k_binary_contents.size(), "size");

        const bool same = bytes.has_value() && std::equal(bytes.get_value().begin(), bytes.get_value().end(), k_binary_contents.begin(), k_binary_contents.end(), [](std::byte b, char ch) { return b == static_cast<std::byte>(ch); });
        c.expect(same, "contents");

        return c;
    }

    //-------------------------------------------------------------------------
    check a_text_file_is_read_without_conversion()
    {
        check c;
        const xvfs_test::temp_directory directory;
        directory.write_file("config/game.json", k_binary_contents);
        const xvfs::context vfs = make_context(directory);

        const xvfs::result<std::string> text = xvfs::read_text_file(vfs, "test:/config/game.json");
        c.expect(text.has_value() && text.get_value() == k_binary_contents, "contents");

        return c;
    }

    //-------------------------------------------------------------------------
    check an_empty_file_reads_as_empty()
    {
        check c;
        const xvfs_test::temp_directory directory;
        directory.write_file("empty.txt", "");
        const xvfs::context vfs = make_context(directory);

        const xvfs::result<std::string> text = xvfs::read_text_file(vfs, "test:/empty.txt");
        c.expect(text.has_value() && text.get_value().empty(), "text");

        const xvfs::result<std::vector<std::byte>> bytes = xvfs::read_binary_file(vfs, "test:/empty.txt");
        c.expect(bytes.has_value() && bytes.get_value().empty(), "bytes");

        return c;
    }

    //-------------------------------------------------------------------------
    check a_missing_file_or_a_directory_is_not_a_file()
    {
        check c;
        const xvfs_test::temp_directory directory;
        directory.write_file("maps/harbor.map", "x");

        std::vector<std::string> errors;
        xvfs::context vfs = make_context(directory);
        vfs.set_log_callback([&](xvfs::log_level level, std::string_view message) {
            if (level == xvfs::log_level::error)
            {
                errors.emplace_back(message);
            }
        });

        c.expect(xvfs::read_text_file(vfs, "test:/missing.txt").get_error() == error_code::not_a_file, "missing");
        c.expect(xvfs::read_binary_file(vfs, "test:/maps").get_error() == error_code::not_a_file, "directory");
        c.expect(errors.size() == 2, "both are logged as errors");
        c.expect(!errors.empty() && errors[0].find("missing.txt") != std::string::npos, "the log names the file");

        return c;
    }

    //-------------------------------------------------------------------------
    check reads_report_resolve_errors()
    {
        check c;
        const xvfs::context empty;
        c.expect(xvfs::read_text_file(empty, "test:/a.txt").get_error() == error_code::no_wildcards, "no wildcards");
        c.expect(!xvfs::is_file(empty, "test:/a.txt"), "is_file is false");
        c.expect(xvfs::create_directories(empty, "test:/a") == error_code::no_wildcards, "create_directories");

        return c;
    }

    //-------------------------------------------------------------------------
    check files_and_directories_are_told_apart()
    {
        check c;
        const xvfs_test::temp_directory directory;
        directory.write_file("maps/harbor.map", "x");
        const xvfs::context vfs = make_context(directory);

        c.expect(xvfs::is_file(vfs, "test:/maps/harbor.map"), "file is a file");
        c.expect(!xvfs::is_file(vfs, "test:/maps"), "directory is not a file");
        c.expect(xvfs::is_directory(vfs, "test:/maps"), "directory is a directory");
        c.expect(!xvfs::is_directory(vfs, "test:/maps/harbor.map"), "file is not a directory");
        c.expect(!xvfs::is_file(vfs, "test:/nothing") && !xvfs::is_directory(vfs, "test:/nothing"), "missing is neither");

        return c;
    }

    //-------------------------------------------------------------------------
    check create_directories_makes_every_missing_parent()
    {
        check c;
        const xvfs_test::temp_directory directory;
        const xvfs::context vfs = make_context(directory);

        c.expect(xvfs::create_directories(vfs, "test:/saves/slot_1/checkpoints") == error_code::none, "created");
        c.expect(xvfs::is_directory(vfs, "test:/saves/slot_1/checkpoints"), "exists");
        c.expect(xvfs::create_directories(vfs, "test:/saves/slot_1") == error_code::none, "an existing directory is fine");

        directory.write_file("blocker", "x");
        c.expect(xvfs::create_directories(vfs, "test:/blocker/inner") == error_code::filesystem_error, "a file in the way fails");

        return c;
    }

    //-------------------------------------------------------------------------
    check reads_use_the_callers_allocator()
    {
        check c;
        const xvfs_test::temp_directory directory;
        const std::string_view contents = "0123456789abcdefghijklmnopqrstuvwxyz";
        directory.write_file("blob.bin", contents);
        const xvfs::context vfs = make_context(directory);

        int32_t count = 0;
        const counting_allocator<std::byte> byte_allocator(&count);
        const auto bytes = xvfs::read_binary_file(vfs, "test:/blob.bin", byte_allocator);
        c.expect(bytes.has_value() && bytes.get_value().size() == contents.size(), "read");
        c.expect(count == 1, "one allocation through the allocator");

        const counting_allocator<char> char_allocator(&count);
        const auto text = xvfs::read_text_file(vfs, "test:/blob.bin", char_allocator);
        c.expect(text.has_value() && text.get_value().size() == contents.size(), "read text");
        c.expect(count == 2, "text used the allocator too");

        return c;
    }

    //-------------------------------------------------------------------------
    check an_input_file_reads_into_caller_memory()
    {
        check c;
        const xvfs_test::temp_directory directory;
        directory.write_file("blob.bin", k_letters);
        const xvfs::context vfs = make_context(directory);

        xvfs::result<xvfs::input_file> opened = xvfs::open_input_file(vfs, "test:/blob.bin");
        c.expect(opened.has_value(), "opened");
        if (!opened.has_value())
        {
            return c;
        }

        xvfs::input_file file = std::move(opened).get_value();
        c.expect(file.get_size() == k_letters.size(), "size");
        c.expect(file.get_path().ends_with("/blob.bin"), "path");

        std::array<std::byte, k_short_read> head{};
        c.expect(file.read_into(head) == error_code::none, "a shorter read works");
        c.expect(head[0] == std::byte{'a'} && head.back() == std::byte{'c'}, "from the start");

        std::array<std::byte, k_long_read> too_long{};
        c.expect(file.read_into(too_long) == error_code::read_failed, "reading past the end fails");

        std::array<std::byte, k_letters.size()> again{};
        c.expect(file.read_into(again) == error_code::none && again.back() == std::byte{'f'}, "the file is still usable");

        return c;
    }

    constexpr std::array k_tests = {
        xvfs_test::test_case{.name = "a_binary_file_is_read_byte_for_byte", .run = a_binary_file_is_read_byte_for_byte},
        xvfs_test::test_case{.name = "a_text_file_is_read_without_conversion", .run = a_text_file_is_read_without_conversion},
        xvfs_test::test_case{.name = "an_empty_file_reads_as_empty", .run = an_empty_file_reads_as_empty},
        xvfs_test::test_case{.name = "a_missing_file_or_a_directory_is_not_a_file", .run = a_missing_file_or_a_directory_is_not_a_file},
        xvfs_test::test_case{.name = "reads_report_resolve_errors", .run = reads_report_resolve_errors},
        xvfs_test::test_case{.name = "files_and_directories_are_told_apart", .run = files_and_directories_are_told_apart},
        xvfs_test::test_case{.name = "create_directories_makes_every_missing_parent", .run = create_directories_makes_every_missing_parent},
        xvfs_test::test_case{.name = "reads_use_the_callers_allocator", .run = reads_use_the_callers_allocator},
        xvfs_test::test_case{.name = "an_input_file_reads_into_caller_memory", .run = an_input_file_reads_into_caller_memory},
    };
} // namespace

namespace xvfs_test
{
    //-------------------------------------------------------------------------
    std::span<const test_case> get_fileio_tests()
    {
        return k_tests;
    }
} // namespace xvfs_test
