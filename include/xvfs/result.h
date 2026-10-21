#pragma once

#include "xvfs/error.h"

#include <concepts>
#include <optional>
#include <utility>

/// @file
/// @brief xvfs::result, a value or an error code (a small stand-in for C++23 std::expected).
///
/// @code
/// const xvfs::result<std::string> path = vfs.resolve_path("content:/maps/harbor.map");
/// if (!path.has_value())
/// {
///     report(xvfs::get_error_message(path.get_error()));
///     return;
/// }
/// open(path.get_value());
/// @endcode
///
/// Ownership: a result owns its value. Use std::move(r).get_value() to take it out.
/// Threading: a result is a plain value; const access is safe from any thread.

namespace xvfs
{
    /// @brief Holds either a value of type T or an error_code that says why there is none.
    ///
    /// Contract: get_value() may only be called when has_value() is true, and get_error()
    /// returns error_code::none in that case. Calling get_value() without a value is a
    /// programmer error and undefined behaviour; xvfs does not check it.
    /// @tparam T The value type. It must be movable.
    template <typename T>
        requires std::movable<T>
    class [[nodiscard]] result
    {
    public:
        /// @brief Makes a result that holds a value.
        /// @param value The value; it is moved in.
        explicit result(T value);

        /// @brief Makes a result that holds an error.
        /// @param error Why there is no value. Must not be error_code::none.
        explicit result(error_code error);

        /// @brief Says whether the result holds a value.
        /// @return True when there is a value, false when there is an error.
        bool has_value() const;

        /// @brief Gives the error.
        /// @return The error, or error_code::none when the result holds a value.
        error_code get_error() const;

        /// @brief Gives read access to the value. Requires has_value().
        /// @return A reference that lives as long as this result.
        const T& get_value() const&;

        /// @brief Moves the value out of a result that is about to go away. Requires has_value().
        /// @return The value, by value, so it cannot dangle.
        T get_value() &&;

    private:
        std::optional<T> m_value;
        error_code m_error;
    };

    template <typename T>
        requires std::movable<T>
    result<T>::result(T value)
        : m_value(std::move(value))
        , m_error(error_code::none)
    {
    }

    template <typename T>
        requires std::movable<T>
    result<T>::result(error_code error)
        : m_value()
        , m_error(error)
    {
    }

    template <typename T>
        requires std::movable<T>
    bool result<T>::has_value() const
    {
        return m_value.has_value();
    }

    template <typename T>
        requires std::movable<T>
    error_code result<T>::get_error() const
    {
        return m_error;
    }

    template <typename T>
        requires std::movable<T>
    const T& result<T>::get_value() const&
    {
        // The caller checked has_value(); see the class contract.
        return *m_value; // NOLINT(bugprone-unchecked-optional-access): documented precondition, as for std::optional
    }

    template <typename T>
        requires std::movable<T>
    T result<T>::get_value() &&
    {
        // The caller checked has_value(); see the class contract.
        return std::move(*m_value); // NOLINT(bugprone-unchecked-optional-access): documented precondition, as for std::optional
    }
} // namespace xvfs
