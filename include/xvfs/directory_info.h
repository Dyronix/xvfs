#pragma once

#include <string>
#include <string_view>

/// @file
/// @brief Splits a directory path into its parent directory and its own name.
///
/// @code
/// const xvfs::directory_info_view view("content:/maps/harbor");
/// // view.get_path() == "content:/maps", view.get_name() == "harbor"
/// @endcode
///
/// Rules: `/` and `\` both separate directories. The name is the text after the last
/// separator, so `content:/maps/` has the parent `content:/maps` and an empty name.
///
/// Ownership: directory_info_view holds views into the caller's text, which must outlive it.
/// directory_info owns copies. Threading: both are plain values; const access is safe from
/// any thread. Failure: parsing cannot fail; any text gives some split.

namespace xvfs
{
    /// @brief A non-owning split of a directory path. The viewed text must outlive the view.
    class directory_info_view
    {
    public:
        /// @brief Splits a directory path. See the file comment for the rules.
        /// @param full_path The path to split; it is viewed, not copied.
        explicit directory_info_view(std::string_view full_path);

        /// @brief Gives the parent directory, without a trailing separator.
        /// @return The parent, or an empty view for a single segment.
        std::string_view get_path() const;

        /// @brief Gives the last segment.
        /// @return The directory name; empty when the path ends in a separator.
        std::string_view get_name() const;

    private:
        std::string_view m_path;
        std::string_view m_name;
    };

    //-------------------------------------------------------------------------

    /// @brief An owning split of a directory path.
    class directory_info
    {
    public:
        /// @brief Splits a directory path and copies the parts. See the file comment for the rules.
        /// @param full_path The path to split.
        explicit directory_info(std::string_view full_path);

        /// @brief Gives the parent directory, without a trailing separator.
        /// @return A view that lives as long as this object.
        std::string_view get_path() const;

        /// @brief Gives the last segment.
        /// @return A view that lives as long as this object.
        std::string_view get_name() const;

        /// @brief Joins the parts again with `/`.
        /// @return The same as make_directory_path(get_path(), get_name()).
        std::string make_full_path() const;

    private:
        std::string m_path;
        std::string m_name;
    };

    /// @brief Joins a parent directory and a directory name into one path.
    ///
    /// Adds a `/` after a non-empty parent that does not end in one, and turns every `\`
    /// into `/`.
    /// @param path The parent directory; may be empty.
    /// @param name The directory name.
    /// @return The joined path.
    std::string make_directory_path(std::string_view path, std::string_view name);
} // namespace xvfs
