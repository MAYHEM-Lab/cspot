//
// Created by fatih on 5/3/18.
//

#pragma once

#include <tos/utility.hpp>
#include <memory>
#include <type_traits>
#include <tos/compiler.hpp>
#include <new>
#include <nonstd/expected.hpp>
#include <optional>

/**
 * This function is called upon a force_get execution on an
 * expected object that's in unexpected state.
 *
 * This function is implemented as a weak symbol on supported
 * platforms and could be replaced by user provided
 * implementations.
 *
 * The implementation must never return to the caller, otherwise
 * the behaviour is undefined.
 */
[[noreturn]] void tos_force_get_failed(void*);

namespace tos
{
    template <class ErrT>
    struct unexpected_t {
        ErrT m_err;
    };

    /**
     * This function ttemplate is used to disambiguate an error from a success value
     * when the type of the error and the expected is the same:
     *
     *      expected<int, int> find(...)
     *      {
     *          if (found)
     *          {
     *              return index;
     *          }
     *          // return -1; -> would signal success
     *          return unexpected(-1); // signals error
     *      }
     *
     * @param err the error object
     * @tparam ErrT type of the error object
     */
    template <class ErrT>
    auto unexpected(ErrT&& err)
    {
        return unexpected_t<ErrT>{ std::forward<ErrT>(err) };
    }

    template <class T, class ErrT>
    class expected;

    template <class T>
    struct is_expected : std::false_type {};

    template <class U, class V>
    struct is_expected<expected<U, V>> : std::true_type {};

    template <class T, class ErrT>
    class expected
    {
        using internal_t = tl::expected<T, ErrT>;
    public:
        template <class U = T, typename = std::enable_if_t<std::is_same<U, void>{}>>
        constexpr expected() : m_internal{} {}

        template <class U = T, typename = std::enable_if_t<!is_expected<std::remove_reference_t <U>>{}>>
        constexpr expected(U&& u) : m_internal{std::forward<U>(u)} {}

        template <class ErrU>
        constexpr expected(unexpected_t<ErrU>&& u) : m_internal{tl::make_unexpected(std::move(u.m_err))} {}

        constexpr explicit PURE operator bool() const { return bool(m_internal); }

        constexpr bool operator!() {
            return !bool(m_internal);
        }

        template <class ResT, typename = std::enable_if_t<!std::is_same_v<T, ResT>>>
        constexpr operator expected<ResT, ErrT>() const
        {
            if (*this)
            {
                tos_force_get_failed(nullptr);
                __builtin_unreachable();
            }

            return unexpected(m_internal.error());
        }

        explicit constexpr operator std::optional<T>() const
        {
          if (!*this)
          {
            return std::nullopt;
          }

          return force_get(*this);
        }

        using value_type = typename internal_t::value_type;
        using error_type = typename internal_t::error_type;
    private:
        internal_t m_internal;

        template <class ExpectedT, class HandlerT, class ErrHandlerT>
        friend auto with(ExpectedT&& e, HandlerT&& have_handler, ErrHandlerT&&)
            -> decltype(have_handler(std::forward<decltype(*e.m_internal)>(*e.m_internal)));

        template <class ExpectedT>
        friend decltype(auto) force_get(ExpectedT&&);
        template <class ExpectedT>
        friend decltype(auto) force_error(ExpectedT&&);
    };

    template <class ExpectedT, class HandlerT, class ErrHandlerT>
    auto ALWAYS_INLINE with(ExpectedT&& e, HandlerT&& have_handler, ErrHandlerT&& err_handler)
        -> decltype(have_handler(std::forward<decltype(*e.m_internal)>(*e.m_internal)))
    {
        if (e)
        {
            return have_handler(std::forward<decltype(*e.m_internal)>(*e.m_internal));
        }
        return err_handler(std::forward<decltype(e.m_internal.error())>(e.m_internal.error()));
    }

    template <class ExpectedT>
    decltype(auto) ALWAYS_INLINE force_error(ExpectedT&& e)
    {
        if (!e)
        {
            return e.m_internal.error();
        }

        tos_force_get_failed(nullptr);
    }

    template <class ExpectedT>
    decltype(auto) ALWAYS_INLINE force_get(ExpectedT&& e)
    {
        if (e)
        {
            return std::forward<decltype(*e.m_internal)>(*e.m_internal);
        }

        tos_force_get_failed(nullptr);
    }

    template <class T, class U>
    typename T::value_type get_or(T&& t, U&& r)
    {
        return with(std::forward<T>(t),
                [](auto&& res) -> decltype(auto) { return std::forward<decltype(res)>(res); },
                [&](auto&&) -> decltype(auto) { return r; });
    }

    struct ignore_t
    {
        template <class T>
        void operator()(T&&)const{}
    };
    static constexpr ignore_t ignore{};
}
