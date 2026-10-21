#include "test_check.h"
#include "xvfs/directory_info.h"
#include "xvfs/file_info.h"
#include "xvfs/path.h"

#include <array>
#include <string>
#include <string_view>

namespace
{
    using xvfs_test::check;

    //-------------------------------------------------------------------------
    check a_file_path_splits_into_three_parts()
    {
        check c;
        const xvfs::file_info_view view("local:/content/shaders/lit.vs");
        c.expect(view.get_path() == "local:/content/shaders", "path");
        c.expect(view.get_filename() == "lit", "filename");
        c.expect(view.get_extension() == ".vs", "extension");

        return c;
    }

    //-------------------------------------------------------------------------
    check only_the_last_dot_of_the_last_segment_starts_the_extension()
    {
        check c;
        const xvfs::file_info_view archive("a/b/archive.tar.gz");
        c.expect(archive.get_filename() == "archive.tar" && archive.get_extension() == ".gz", "last dot");

        const xvfs::file_info_view dotted_directory("data.v2/readme");
        c.expect(dotted_directory.get_path() == "data.v2", "path keeps the dot");
        c.expect(dotted_directory.get_filename() == "readme" && dotted_directory.get_extension().empty(), "no extension");

        const xvfs::file_info_view relative("./data/file");
        c.expect(relative.get_path() == "./data" && relative.get_filename() == "file" && relative.get_extension().empty(), "leading dot directory");

        const xvfs::file_info_view hidden("config/.gitignore");
        c.expect(hidden.get_filename().empty() && hidden.get_extension() == ".gitignore", "dot file");

        const xvfs::file_info_view bare("name.txt");
        c.expect(bare.get_path().empty() && bare.get_filename() == "name", "no directory");

        return c;
    }

    //-------------------------------------------------------------------------
    check backslashes_separate_directories()
    {
        check c;
        const xvfs::file_info_view view("C:\\game\\maps\\harbor.map");
        c.expect(view.get_path() == "C:\\game\\maps", "path");
        c.expect(view.get_filename() == "harbor", "filename");

        const xvfs::directory_info_view directory("C:\\game\\maps");
        c.expect(directory.get_path() == "C:\\game" && directory.get_name() == "maps", "directory");

        return c;
    }

    //-------------------------------------------------------------------------
    check an_owning_file_info_joins_with_forward_slashes()
    {
        check c;
        const xvfs::file_info info(std::string("C:\\game\\maps\\harbor.map"));
        c.expect(info.get_filename() == "harbor", "owns the parts");
        c.expect(info.make_full_path() == "C:/game/maps/harbor.map", "joined");

        c.expect(xvfs::make_file_path("a/b", "c", ".d") == "a/b/c.d", "adds a separator");
        c.expect(xvfs::make_file_path("a/b/", "c", ".d") == "a/b/c.d", "keeps one separator");
        c.expect(xvfs::make_file_path("a\\b\\", "c", ".d") == "a/b/c.d", "a trailing backslash counts");
        c.expect(xvfs::make_file_path("", "c", ".d") == "c.d", "no directory");

        return c;
    }

    //-------------------------------------------------------------------------
    check a_directory_path_splits_into_parent_and_name()
    {
        check c;
        const xvfs::directory_info_view view("local:/content/shaders");
        c.expect(view.get_path() == "local:/content" && view.get_name() == "shaders", "split");

        const xvfs::directory_info_view single("content");
        c.expect(single.get_path().empty() && single.get_name() == "content", "single segment");

        const xvfs::directory_info_view trailing("content/maps/");
        c.expect(trailing.get_path() == "content/maps" && trailing.get_name().empty(), "trailing separator");

        const xvfs::directory_info info("a\\b\\c");
        c.expect(info.make_full_path() == "a/b/c", "joined");
        c.expect(xvfs::make_directory_path("a", "b") == "a/b", "make");

        return c;
    }

    //-------------------------------------------------------------------------
    check normalizing_only_changes_backslashes()
    {
        check c;
        c.expect(xvfs::make_normalized_path("a\\b/c\\d.e") == "a/b/c/d.e", "normalized");
        c.expect(xvfs::make_normalized_path("").empty(), "empty");

        return c;
    }

    constexpr std::array k_tests = {
        xvfs_test::test_case{.name = "a_file_path_splits_into_three_parts", .run = a_file_path_splits_into_three_parts},
        xvfs_test::test_case{.name = "only_the_last_dot_of_the_last_segment_starts_the_extension", .run = only_the_last_dot_of_the_last_segment_starts_the_extension},
        xvfs_test::test_case{.name = "backslashes_separate_directories", .run = backslashes_separate_directories},
        xvfs_test::test_case{.name = "an_owning_file_info_joins_with_forward_slashes", .run = an_owning_file_info_joins_with_forward_slashes},
        xvfs_test::test_case{.name = "a_directory_path_splits_into_parent_and_name", .run = a_directory_path_splits_into_parent_and_name},
        xvfs_test::test_case{.name = "normalizing_only_changes_backslashes", .run = normalizing_only_changes_backslashes},
    };
} // namespace

namespace xvfs_test
{
    //-------------------------------------------------------------------------
    std::span<const test_case> get_path_tests()
    {
        return k_tests;
    }
} // namespace xvfs_test
