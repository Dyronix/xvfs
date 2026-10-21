#pragma once

#include <cstdint>
#include <filesystem>
#include <source_location>
#include <span>
#include <string_view>

// A tiny test runner, so xvfs needs no test framework and no download at configure time.
// Usage: a test is a function that makes a check, calls expect() on it and returns it.
//     xvfs_test::check an_empty_path_is_an_error() { xvfs_test::check c; c.expect(...); return c; }
// Single threaded: a check belongs to the thread that made it.

namespace xvfs_test
{
    // Collects the failures of one test and prints each one when it happens.
    class check
    {
    public:
        check();

        // Records a failure, with the caller's file and line, when condition is false.
        void expect(bool condition, std::string_view what, std::source_location where = std::source_location::current());

        int32_t get_failure_count() const;

    private:
        int32_t m_failure_count;
    };

    // A fresh directory under the system temp directory, removed with its contents on destruction.
    class temp_directory
    {
    public:
        temp_directory();
        ~temp_directory();

        temp_directory(const temp_directory&) = delete;
        temp_directory& operator=(const temp_directory&) = delete;
        temp_directory(temp_directory&&) = delete;
        temp_directory& operator=(temp_directory&&) = delete;

        const std::filesystem::path& get_path() const;

        // Writes a file below the directory and returns its full path.
        std::filesystem::path write_file(std::string_view relative_path, std::string_view contents) const;

    private:
        std::filesystem::path m_path;
    };

    // One named test.
    struct test_case
    {
        std::string_view name;
        check (*run)();
    };

    // One group of tests, run by name from the command line.
    struct test_group
    {
        std::string_view name;
        std::span<const test_case> tests;
    };

    // Runs every test of the groups whose name matches filter (all groups for an empty filter).
    // Returns the process exit code: 0 when every test passed.
    int32_t run_groups(std::span<const test_group> groups, std::string_view filter);

    std::span<const test_case> get_context_tests();
    std::span<const test_case> get_fileio_tests();
    std::span<const test_case> get_path_tests();
    std::span<const test_case> get_threading_tests();
} // namespace xvfs_test
