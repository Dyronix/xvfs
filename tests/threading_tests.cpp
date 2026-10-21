#include "test_check.h"
#include "xvfs/context.h"
#include "xvfs/fileio.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace
{
    using xvfs_test::check;

    constexpr int32_t k_thread_count = 8;
    constexpr int32_t k_resolves_per_thread = 20000;
    constexpr int32_t k_reads_per_thread = 50;
    constexpr std::size_t k_buffer_size = 128;

    struct expected_path
    {
        std::string_view input;
        std::string_view output;
    };

    constexpr std::array k_paths = {
        expected_path{.input = "content:/maps/harbor.map", .output = "/game/content/maps/harbor.map"},
        expected_path{.input = "content:dlc/maps/pier.map", .output = "/game/dlc/maps/pier.map"},
        expected_path{.input = "saves:\\slot_1\\checkpoint.bin", .output = "/home/player/saves/slot_1/checkpoint.bin"},
        expected_path{.input = "/absolute/file.txt", .output = "/absolute/file.txt"},
    };

    //-------------------------------------------------------------------------
    xvfs::context make_registered_context()
    {
        xvfs::context vfs;
        const bool added = vfs.add_wildcard("content:", "/game/content") == xvfs::error_code::none && vfs.add_wildcard("content:dlc", "/game/dlc") == xvfs::error_code::none && vfs.add_wildcard("saves:", "/home/player/saves") == xvfs::error_code::none;
        static_cast<void>(added);

        return vfs;
    }

    //-------------------------------------------------------------------------
    check many_threads_resolve_one_const_context()
    {
        check c;
        const xvfs::context vfs = make_registered_context();
        std::atomic<int32_t> mismatches{0};

        std::vector<std::thread> threads;
        threads.reserve(k_thread_count);
        for (int32_t t = 0; t < k_thread_count; ++t)
        {
            threads.emplace_back([&vfs, &mismatches, t]() {
                std::array<char, k_buffer_size> buffer{};
                for (int32_t i = 0; i < k_resolves_per_thread; ++i)
                {
                    const expected_path& expected = k_paths[static_cast<std::size_t>(i + t) % k_paths.size()];
                    const xvfs::result<std::string> owned = vfs.resolve_path(expected.input);
                    const xvfs::result<std::string_view> viewed = vfs.resolve_path_into(expected.input, buffer);
                    const bool same = owned.has_value() && viewed.has_value() && owned.get_value() == expected.output && viewed.get_value() == expected.output;
                    if (!same)
                    {
                        mismatches.fetch_add(1);
                    }
                }
            });
        }
        for (std::thread& thread : threads)
        {
            thread.join();
        }

        c.expect(mismatches.load() == 0, "every thread saw the same answers");

        return c;
    }

    //-------------------------------------------------------------------------
    check many_threads_read_files_and_log_through_one_context()
    {
        check c;
        const xvfs_test::temp_directory directory;
        directory.write_file("shared.txt", "shared contents");

        std::mutex log_mutex;
        int32_t logged = 0;
        xvfs::context vfs;
        vfs.set_log_callback([&](xvfs::log_level, std::string_view) {
            const std::lock_guard<std::mutex> lock(log_mutex);
            ++logged;
        });
        c.expect(vfs.add_wildcard("test:", directory.get_path().generic_string()) == xvfs::error_code::none, "add");
        const xvfs::context& shared = vfs;

        std::atomic<int32_t> failures{0};
        std::vector<std::thread> threads;
        threads.reserve(k_thread_count);
        for (int32_t t = 0; t < k_thread_count; ++t)
        {
            threads.emplace_back([&shared, &failures]() {
                for (int32_t i = 0; i < k_reads_per_thread; ++i)
                {
                    const xvfs::result<std::string> text = xvfs::read_text_file(shared, "test:/shared.txt");
                    const bool missing_fails = xvfs::read_text_file(shared, "test:/missing.txt").get_error() == xvfs::error_code::not_a_file;
                    if (!text.has_value() || text.get_value() != "shared contents" || !missing_fails)
                    {
                        failures.fetch_add(1);
                    }
                }
            });
        }
        for (std::thread& thread : threads)
        {
            thread.join();
        }

        c.expect(failures.load() == 0, "every read worked");
        // One info line for the wildcard, then a debug line per open and an error line per miss.
        c.expect(logged == 1 + 2 * k_thread_count * k_reads_per_thread, "every message arrived");

        return c;
    }

    constexpr std::array k_tests = {
        xvfs_test::test_case{.name = "many_threads_resolve_one_const_context", .run = many_threads_resolve_one_const_context},
        xvfs_test::test_case{.name = "many_threads_read_files_and_log_through_one_context", .run = many_threads_read_files_and_log_through_one_context},
    };
} // namespace

namespace xvfs_test
{
    //-------------------------------------------------------------------------
    std::span<const test_case> get_threading_tests()
    {
        return k_tests;
    }
} // namespace xvfs_test
