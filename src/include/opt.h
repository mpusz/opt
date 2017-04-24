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

template<typename T>
struct opt_default_policy {
  static constexpr bool has_value(const T& value) noexcept = delete;
  static constexpr T null_value() noexcept = delete;
};

template<typename T, T NullValue>
struct opt_null_value_policy {
  static constexpr T null_value() noexcept { return NullValue; }
};

template<typename T, typename NullType>
struct opt_null_type_policy {
  static constexpr bool has_value(T value) noexcept { return value != null_value(); }
  static constexpr T null_value() noexcept { return NullType::value; }
};

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
constexpr bool has_value(std::true_type, const T& value) noexcept(noexcept(Policy::has_value(std::declval<T>())))
{
  return Policy::has_value(value);
}

template<typename Policy, typename T>
constexpr bool has_value(std::false_type, const T& value) noexcept(noexcept(Policy::null_value()))
{
  return !(value == Policy::null_value());
}

} // namespace detail

template<typename T, typename Policy = opt_default_policy<T>>
class opt {
  T value_;

public:
  using value_type = T;

  // constructors
  constexpr opt() noexcept(noexcept(Policy::null_value())) : value_{Policy::null_value()} {}

  constexpr opt(std::experimental::nullopt_t) noexcept(noexcept(Policy::null_value())) : opt{} {}

  opt(const opt&) = default;
  opt(opt&&) = default;

  template<typename... Args, detail::Requires<std::is_constructible<T, Args...>> = true>
  constexpr explicit opt(std::experimental::in_place_t, Args&&... args) : value_{std::forward<Args>(args)...}
  {
  }

  template<typename U, typename... Args,
           detail::Requires<std::is_constructible<T, std::initializer_list<U>&, Args&&...>> = true>
  constexpr explicit opt(std::experimental::in_place_t, std::initializer_list<U> ilist, Args&&... args)
      : value_{ilist, std::forward<Args>(args)...}
  {
  }

  template<typename U = T,
           detail::Requires<std::is_constructible<T, U&&>,
                            std::negation<std::is_same<std::decay_t<U>, std::experimental::in_place_t>>,
                            std::negation<std::is_same<opt<T, Policy>, std::decay_t<U>>>,
                            std::negation<detail::is_opt<std::decay_t<U>>>> = true,
           detail::Requires<std::negation<std::is_convertible<U&&, T>>> = true>
  explicit constexpr opt(U&& value) : value_{std::forward<U>(value)}
  {
  }

  template<typename U = T,
           detail::Requires<std::is_constructible<T, U&&>,
                            std::negation<std::is_same<std::decay_t<U>, std::experimental::in_place_t>>,
                            std::negation<std::is_same<opt<T, Policy>, std::decay_t<U>>>,
                            std::negation<detail::is_opt<std::decay_t<U>>>> = true,
           detail::Requires<std::is_convertible<U&&, T>> = true>
  constexpr opt(U&& value) : value_{std::forward<U>(value)}
  {
  }

  template<typename U, typename P,
           detail::Requires<std::is_constructible<T, const U&>,
                            std::disjunction<std::is_same<std::decay_t<T>, bool>, std::negation<detail::constructs_or_converts_from_opt<T, U, P>>>> = true,
           detail::Requires<std::negation<std::is_convertible<const U&, T>>> = true>
  explicit opt(const opt<U, P>& other) : value_{other.has_value() ? T{*other} : Policy::null_value()} {}

  template<typename U, typename P,
           detail::Requires<std::is_constructible<T, const U&>,
                            std::disjunction<std::is_same<std::decay_t<T>, bool>, std::negation<detail::constructs_or_converts_from_opt<T, U, P>>>> = true,
           detail::Requires<std::is_convertible<const U&, T>> = true>
  opt(const opt<U, P>& other) : value_{other.has_value() ? T{*other} : Policy::null_value()} {}

  template<typename U, typename P,
           detail::Requires<std::is_constructible<T, U&&>,
                            std::disjunction<std::is_same<std::decay_t<T>, bool>, std::negation<detail::constructs_or_converts_from_opt<T, U, P>>>> = true,
           detail::Requires<std::negation<std::is_convertible<U&&, T>>> = true>
  explicit constexpr opt(opt<U, P>&& other) : value_{other.has_value() ? T{std::move(*other)} : Policy::null_value()} {}

  template<typename U, typename P,
           detail::Requires<std::is_constructible<T, U&&>,
                            std::disjunction<std::is_same<std::decay_t<T>, bool>, std::negation<detail::constructs_or_converts_from_opt<T, U, P>>>> = true,
           detail::Requires<std::is_convertible<U&&, T>> = true>
  constexpr opt(opt<U, P>&& other) : value_{other.has_value() ? T{std::move(*other)} : Policy::null_value()} {}

  // assignment
  opt& operator=(std::experimental::nullopt_t) noexcept
  {
    reset();
    return *this;
  }
  opt& operator=(const opt& other) = default;
  opt& operator=(opt&& other) = default;

  template<typename U = T,
           detail::Requires<std::negation<std::is_same<opt<T, Policy>, std::decay<U>>>,
                            std::negation<std::conjunction<std::is_scalar<T>, std::is_same<T, std::decay_t<U>>>>,
                            std::is_constructible<T, U>, std::is_assignable<T&, U>> = true>
  opt& operator=(U&& value)
  {
    **this = std::forward<U>(value);
    return *this;
  }

  template<typename U, typename P,
           detail::Requires<std::is_constructible<T, const U&>, std::is_assignable<T&, const U&>,
                            std::negation<detail::constructs_or_converts_from_opt<T, U, P>>,
                            std::negation<detail::assigns_from_opt<T, U, P>>> = true>
  opt& operator=(const opt<U, P>& other)
  {
    if(other.has_value())
      *this = *other;
    else
      reset();
    return *this;
  }

  template<typename U, typename P,
           detail::Requires<std::is_constructible<T, U>, std::is_assignable<T&, U>,
                            std::negation<detail::constructs_or_converts_from_opt<T, U, P>>,
                            std::negation<detail::assigns_from_opt<T, U, P>>> = true>
  opt& operator=(opt<U, P>&& other)
  {
    if(other.has_value())
      *this = std::move(*other);
    else
      reset();
    return *this;
  }

  // swap
  void swap(opt& other) noexcept(
      std::is_nothrow_move_constructible<T>::value /* && std::is_nothrow_swappable<T>::value */)
  {
    std::swap(value_, other.value_);
  }

  // observers
  constexpr const T* operator->() const { return &value_; }
  constexpr T* operator->() { return &value_; }
  constexpr const T& operator*() const & { return value_; }
  constexpr T& operator*() & { return value_; }
  constexpr T&& operator*() && { return std::move(value_); }
  constexpr const T&& operator*() const && { return std::move(value_); }

  constexpr bool has_value() const noexcept(noexcept(detail::has_value<Policy>(detail::has_has_value<T, Policy>{}, std::declval<T>())))
  {
    return detail::has_value<Policy>(detail::has_has_value<T, Policy>{}, **this);
  }
  constexpr explicit operator bool() const noexcept(noexcept(std::declval<opt<T, Policy>>().has_value()))
  {
    return has_value();
  }

  // clang-format off
  constexpr const T& value() const&              { return has_value() ? **this            : (throw std::experimental::bad_optional_access{}, **this); }
  constexpr T& value() &                         { return has_value() ? **this            : (throw std::experimental::bad_optional_access{}, **this); }
  constexpr T&& value() &&                       { return has_value() ? std::move(**this) : (throw std::experimental::bad_optional_access{}, std::move(**this)); }
  constexpr const T&& value() const&&            { return has_value() ? std::move(**this) : (throw std::experimental::bad_optional_access{}, std::move(**this)); }
  template<typename U>
  constexpr T value_or(U&& default_value) const& { return has_value() ? **this            : T{std::forward<U>(default_value)}; }
  template<typename U>
  constexpr T value_or(U&& default_value) &&     { return has_value() ? std::move(**this) : T{std::forward<U>(default_value)}; }
  // clang-format on

  // modifiers
  void reset() noexcept(noexcept(Policy::null_value())) { **this = Policy::null_value(); }
};

// relational operators
template<typename T, typename P, typename U, typename R>
constexpr bool operator==(const opt<T, P>& lhs, const opt<U, R>& rhs)
{
  return lhs.has_value() == rhs.has_value() && (!lhs.has_value() || *lhs == *rhs);
}

template<typename T, typename P, typename U, typename R>
constexpr bool operator!=(const opt<T, P>& lhs, const opt<U, R>& rhs)
{
  return !(lhs == rhs);
}

template<typename T, typename P, typename U, typename R>
constexpr bool operator<(const opt<T, P>& lhs, const opt<U, R>& rhs)
{
  return (!rhs) ? false : (!lhs) ? true : *lhs < *rhs;
}

template<typename T, typename P, typename U, typename R>
constexpr bool operator>(const opt<T, P>& lhs, const opt<U, R>& rhs)
{
  return (rhs < lhs);
}

template<typename T, typename P, typename U, typename R>
constexpr bool operator<=(const opt<T, P>& lhs, const opt<U, R>& rhs)
{
  return !(rhs < lhs);
}

template<typename T, typename P, typename U, typename R>
constexpr bool operator>=(const opt<T, P>& lhs, const opt<U, R>& rhs)
{
  return !(lhs < rhs);
}

// clang-format off
// comparison with nullopt
template<typename T, typename P> constexpr bool operator==(const opt<T, P>& o, std::experimental::nullopt_t) noexcept { return !o; }
template<typename T, typename P> constexpr bool operator==(std::experimental::nullopt_t, const opt<T, P>& o) noexcept { return !o; }
template<typename T, typename P> constexpr bool operator!=(const opt<T, P>& o, std::experimental::nullopt_t) noexcept { return static_cast<bool>(o); }
template<typename T, typename P> constexpr bool operator!=(std::experimental::nullopt_t, const opt<T, P>& o) noexcept { return static_cast<bool>(o); }
template<typename T, typename P> constexpr bool operator< (const opt<T, P>&,   std::experimental::nullopt_t) noexcept { return false; }
template<typename T, typename P> constexpr bool operator< (std::experimental::nullopt_t, const opt<T, P>& o) noexcept { return static_cast<bool>(o); }
template<typename T, typename P> constexpr bool operator<=(const opt<T, P>& o, std::experimental::nullopt_t) noexcept { return !o; }
template<typename T, typename P> constexpr bool operator<=(std::experimental::nullopt_t, const opt<T, P>&  ) noexcept { return true; }
template<typename T, typename P> constexpr bool operator> (const opt<T, P>& o, std::experimental::nullopt_t) noexcept { return static_cast<bool>(o); }
template<typename T, typename P> constexpr bool operator> (std::experimental::nullopt_t, const opt<T, P>&  ) noexcept { return false; }
template<typename T, typename P> constexpr bool operator>=(const opt<T, P>&,   std::experimental::nullopt_t) noexcept { return true; }
template<typename T, typename P> constexpr bool operator>=(std::experimental::nullopt_t, const opt<T, P>& o) noexcept { return !o; }

// comparison with T
template<typename T, typename P, typename U> constexpr bool operator==(const opt<T, P>& o, const U& value) { return  o && *o == value; }
template<typename T, typename P, typename U> constexpr bool operator==(const U& value, const opt<T, P>& o) { return  o && value == *o; }
template<typename T, typename P, typename U> constexpr bool operator!=(const opt<T, P>& o, const U& value) { return !o || *o != value; }
template<typename T, typename P, typename U> constexpr bool operator!=(const U& value, const opt<T, P>& o) { return !o || value != *o; }
template<typename T, typename P, typename U> constexpr bool operator< (const opt<T, P>& o, const U& value) { return !o || *o < value; }
template<typename T, typename P, typename U> constexpr bool operator< (const U& value, const opt<T, P>& o) { return  o && value < *o; }
template<typename T, typename P, typename U> constexpr bool operator<=(const opt<T, P>& o, const U& value) { return !o || *o <= value; }
template<typename T, typename P, typename U> constexpr bool operator<=(const U& value, const opt<T, P>& o) { return  o && value <= *o; }
template<typename T, typename P, typename U> constexpr bool operator> (const opt<T, P>& o, const U& value) { return  o && *o > value; }
template<typename T, typename P, typename U> constexpr bool operator> (const U& value, const opt<T, P>& o) { return !o || value > *o; }
template<typename T, typename P, typename U> constexpr bool operator>=(const opt<T, P>& o, const U& value) { return  o && *o >= value; }
template<typename T, typename P, typename U> constexpr bool operator>=(const U& value, const opt<T, P>& o) { return !o || value >= *o; }
// clang-format on

// specialized algorithms
namespace std {

template<typename T, typename P>
inline void swap(opt<T, P>& lhs, opt<T, P>& rhs) noexcept(noexcept(lhs.swap(rhs)))
{
  lhs.swap(rhs);
}

// hash support
// template<typename T>
// struct hash;
// template<typename T, typename P>
// struct hash<opt<T, P>>;
}
