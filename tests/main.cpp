#include "test_check.h"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

//-------------------------------------------------------------------------
int main(int argc, char** argv)
{
    const std::array groups = {
        xvfs_test::test_group{.name = "context", .tests = xvfs_test::get_context_tests()},
        xvfs_test::test_group{.name = "fileio", .tests = xvfs_test::get_fileio_tests()},
        xvfs_test::test_group{.name = "path", .tests = xvfs_test::get_path_tests()},
        xvfs_test::test_group{.name = "threading", .tests = xvfs_test::get_threading_tests()},
        xvfs_test::test_group{.name = "utf8", .tests = xvfs_test::get_utf8_tests()},
    };

    const std::span<char*> arguments(argv, static_cast<std::size_t>(argc));
    const std::string_view filter = arguments.size() > 1 ? std::string_view(arguments[1]) : std::string_view();

    return xvfs_test::run_groups(groups, filter);
}
