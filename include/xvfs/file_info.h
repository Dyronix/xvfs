#pragma once

#include <string>
#include <string_view>

/// @file
/// @brief Splits a file path into its directory, file name and extension.
///
/// @code
/// const xvfs::file_info_view view("content:/maps/harbor.map");
/// // view.get_path() == "content:/maps", get_filename() == "harbor", get_extension() == ".map"
/// @endcode
///
/// Rules: `/` and `\` both separate directories. The extension starts at the last `.` of
/// the last segment and includes the dot; a last segment without a dot has no extension.
/// A dot in a directory name is never an extension. `.gitignore` has an empty file name
/// and the extension `.gitignore`.
///
/// Ownership: file_info_view holds views into the caller's text, which must outlive it.
/// file_info owns copies. Threading: both are plain values; const access is safe from any
/// thread. Failure: parsing cannot fail; any text gives some split.

namespace xvfs
{
    /// @brief A non-owning split of a file path. The viewed text must outlive the view.
    class file_info_view
    {
    public:
        /// @brief Splits a file path. See the file comment for the rules.
        /// @param full_path The path to split; it is viewed, not copied.
        explicit file_info_view(std::string_view full_path);

        /// @brief Gives the directory part, without a trailing separator.
        /// @return The directory, or an empty view when the path has no separator.
        std::string_view get_path() const;

        /// @brief Gives the file name without the extension.
        /// @return The file name; empty for a name such as `.gitignore`.
        std::string_view get_filename() const;

        /// @brief Gives the extension including its dot.
        /// @return The extension, such as `.map`, or an empty view.
        std::string_view get_extension() const;

    private:
        std::string_view m_path;
        std::string_view m_filename;
        std::string_view m_extension;
    };

    //-------------------------------------------------------------------------

    /// @brief An owning split of a file path.
    class file_info
    {
    public:
        /// @brief Splits a file path and copies the parts. See the file comment for the rules.
        /// @param full_path The path to split.
        explicit file_info(std::string_view full_path);

        /// @brief Gives the directory part, without a trailing separator.
        /// @return A view that lives as long as this object.
        std::string_view get_path() const;

        /// @brief Gives the file name without the extension.
        /// @return A view that lives as long as this object.
        std::string_view get_filename() const;

        /// @brief Gives the extension including its dot.
        /// @return A view that lives as long as this object.
        std::string_view get_extension() const;

        /// @brief Joins the parts again with `/`.
        /// @return The same as make_file_path(get_path(), get_filename(), get_extension()).
        std::string make_full_path() const;

    private:
        std::string m_path;
        std::string m_filename;
        std::string m_extension;
    };

    /// @brief Joins a directory, a file name and an extension into one path.
    ///
    /// Adds a `/` after a non-empty directory that does not end in one, and turns every `\`
    /// into `/`.
    /// @param path The directory; may be empty.
    /// @param filename The file name without the extension.
    /// @param extension The extension including its dot; may be empty.
    /// @return The joined path.
    std::string make_file_path(std::string_view path, std::string_view filename, std::string_view extension);
} // namespace xvfs
