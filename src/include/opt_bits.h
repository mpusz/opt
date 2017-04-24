// The MIT License (MIT)
//
// Copyright (c) 2016 Mateusz Pusz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <experimental/optional>
#include <type_traits>

template<typename T, typename Policy>
class opt;

namespace detail {

template <typename...>
using void_t = void;

template<typename T, typename P, typename Result, template <class, class> class Operation, class = void>
struct has_operation : std::false_type {
};

template<typename T, typename P, typename Result, template<class, class> class Operation>
struct has_operation<T, P, Result, Operation, void_t<Operation<T, P>>> : std::is_same<Operation<T, P>, Result> {
};

template <typename T, typename P> using has_value_t   = decltype(std::declval<P>().has_value(std::declval<T>()));
template <typename T, typename P> using has_has_value = has_operation<T, P, bool, has_value_t>;

template<typename T>
struct is_opt : std::false_type {};

template<typename T, typename P>
struct is_opt<opt<T, P>> : std::true_type {};

template<typename... Args>
using Requires = std::enable_if_t<std::conjunction<Args...>::value, bool>;

template<typename T, typename U, typename P>
using constructs_or_converts_from_opt =
    std::disjunction<std::is_constructible<T, opt<U, P>&>, std::is_constructible<T, const opt<U, P>&>,
                     std::is_constructible<T, opt<U, P>&&>, std::is_constructible<T, const opt<U, P>&&>,
                     std::is_convertible<opt<U, P>&, T>, std::is_convertible<const opt<U, P>&, T>,
                     std::is_convertible<opt<U, P>&&, T>, std::is_convertible<const opt<U, P>&&, T>>;

template<typename T, typename U, typename P>
using assigns_from_opt =
    std::disjunction<std::is_assignable<T&, opt<U, P>&>, std::is_assignable<T&, const opt<U, P>&>,
                     std::is_assignable<T&, opt<U, P>&&>, std::is_assignable<T&, const opt<U, P>&&>>;

template<typename Policy, typename T>
constexpr bool has_value_impl(std::true_type, const T& value) noexcept(noexcept(Policy::has_value(std::declval<T>())))
{
  return Policy::has_value(value);
}

template<typename Policy, typename T>
constexpr bool has_value_impl(std::false_type, const T& value) noexcept(noexcept(Policy::null_value()))
{
  return !(value == Policy::null_value());
}

template<typename T, typename Policy>
struct opt_policy_traits {
  // always calls Policy::null_value()
  static constexpr auto null_value() noexcept(noexcept(Policy::null_value())) { return Policy::null_value(); }

  // calls Policy::has_value() if available, otherwise uses operator==() for comparison
  static constexpr bool has_value(const T& value) noexcept(noexcept(has_value_impl<Policy>(has_has_value<T, Policy>{}, std::declval<T>())));
};

template<typename T, typename Policy>
constexpr bool opt_policy_traits<T, Policy>::has_value(const T& value) noexcept(noexcept(has_value_impl<Policy>(has_has_value<T, Policy>{}, std::declval<T>())))
{
  return has_value_impl<Policy>(detail::has_has_value<T, Policy>{}, value);
}

} // namespace detail
