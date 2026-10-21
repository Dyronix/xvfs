#include "test_check.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>
#include <system_error>

namespace
{
    // Makes directory names unique within one run; the clock makes them unique across runs.
    std::atomic<uint32_t> g_temp_directory_count{0}; // convention-exception: test-only counter, the one global
} // namespace

namespace xvfs_test
{
    //-------------------------------------------------------------------------
    check::check()
        : m_failure_count(0)
    {
    }

    //-------------------------------------------------------------------------
    void check::expect(bool condition, std::string_view what, std::source_location where)
    {
        if (condition)
        {
            return;
        }

        ++m_failure_count;
        std::printf("    FAILED %s:%u: %.*s\n", where.file_name(), static_cast<unsigned>(where.line()), static_cast<int>(what.size()), what.data());
    }

    //-------------------------------------------------------------------------
    int32_t check::get_failure_count() const
    {
        return m_failure_count;
    }

    //-------------------------------------------------------------------------
    temp_directory::temp_directory()
        : m_path()
    {
        const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
        const uint32_t index = g_temp_directory_count.fetch_add(1);
        const std::string name = "xvfs_test_" + std::to_string(ticks) + "_" + std::to_string(index);

        m_path = std::filesystem::temp_directory_path() / name;
        std::filesystem::create_directories(m_path);
    }

    //-------------------------------------------------------------------------
    temp_directory::~temp_directory()
    {
        std::error_code ignored;
        std::filesystem::remove_all(m_path, ignored);
    }

    //-------------------------------------------------------------------------
    const std::filesystem::path& temp_directory::get_path() const
    {
        return m_path;
    }

    //-------------------------------------------------------------------------
    std::filesystem::path temp_directory::write_file(std::string_view relative_path, std::string_view contents) const
    {
        const std::filesystem::path file = m_path / relative_path;
        std::filesystem::create_directories(file.parent_path());

        std::ofstream stream(file, std::ios::binary);
        stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));

        return file;
    }

    //-------------------------------------------------------------------------
    int32_t run_groups(std::span<const test_group> groups, std::string_view filter)
    {
        int32_t failed_tests = 0;
        int32_t run_tests = 0;
        for (const test_group& group : groups)
        {
            if (!filter.empty() && group.name != filter)
            {
                continue;
            }

            for (const test_case& test : group.tests)
            {
                std::printf("[%.*s] %.*s\n", static_cast<int>(group.name.size()), group.name.data(), static_cast<int>(test.name.size()), test.name.data());
                const check result = test.run();
                ++run_tests;
                if (result.get_failure_count() > 0)
                {
                    ++failed_tests;
                }
            }
        }

        std::printf("%d tests, %d failed\n", run_tests, failed_tests);
        if (run_tests == 0)
        {
            std::printf("no group is named '%.*s'\n", static_cast<int>(filter.size()), filter.data());
            return 1;
        }

        return failed_tests == 0 ? 0 : 1;
    }
} // namespace xvfs_test
