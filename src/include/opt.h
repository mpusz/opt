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

#include "opt_bits.h"

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

template<typename T, typename Policy = opt_default_policy<T>>
class opt {
  using traits = detail::opt_policy_traits<T, Policy>;
  using storage_type = typename traits::storage_type;

  storage_type storage_;

public:
  using value_type = T;

  // constructors
  constexpr opt() noexcept(noexcept(traits::null_value())) : value_{traits::null_value()} {}

  constexpr opt(std::experimental::nullopt_t) noexcept(noexcept(opt<T, Policy>{})) : opt{} {}

  opt(const opt&) = default;
  opt(opt&&) = default;

  template<typename... Args, detail::Requires<std::is_constructible<T, Args...>> = true>
  constexpr explicit opt(std::experimental::in_place_t, Args&&... args) : storage_{std::forward<Args>(args)...}
  {
  }

  template<typename U, typename... Args,
           detail::Requires<std::is_constructible<T, std::initializer_list<U>&, Args&&...>> = true>
  constexpr explicit opt(std::experimental::in_place_t, std::initializer_list<U> ilist, Args&&... args)
      : storage_{ilist, std::forward<Args>(args)...}
  {
  }

  template<typename U = T,
           detail::Requires<std::is_constructible<T, U&&>,
                            std::negation<std::is_same<std::decay_t<U>, std::experimental::in_place_t>>,
                            std::negation<std::is_same<opt<T, Policy>, std::decay_t<U>>>,
                            std::negation<detail::is_opt<std::decay_t<U>>>> = true,
           detail::Requires<std::negation<std::is_convertible<U&&, T>>> = true>
  explicit constexpr opt(U&& value) : storage_{std::forward<U>(value)}
  {
  }

  template<typename U = T,
           detail::Requires<std::is_constructible<T, U&&>,
                            std::negation<std::is_same<std::decay_t<U>, std::experimental::in_place_t>>,
                            std::negation<std::is_same<opt<T, Policy>, std::decay_t<U>>>,
                            std::negation<detail::is_opt<std::decay_t<U>>>> = true,
           detail::Requires<std::is_convertible<U&&, T>> = true>
  constexpr opt(U&& value) : storage_{std::forward<U>(value)}
  {
  }

  template<typename U, typename P,
           detail::Requires<std::is_constructible<T, const U&>,
                            std::disjunction<std::is_same<std::decay_t<T>, bool>,
                                             std::negation<detail::constructs_or_converts_from_opt<T, U, P>>>> = true,
           detail::Requires<std::negation<std::is_convertible<const U&, T>>> = true>
  explicit opt(const opt<U, P>& other) : storage_{other.has_value() ? T{*other} : traits::null_value()}
  {
  }

  template<typename U, typename P,
           detail::Requires<std::is_constructible<T, const U&>,
                            std::disjunction<std::is_same<std::decay_t<T>, bool>,
                                             std::negation<detail::constructs_or_converts_from_opt<T, U, P>>>> = true,
           detail::Requires<std::is_convertible<const U&, T>> = true>
  opt(const opt<U, P>& other) : storage_{other.has_value() ? T{*other} : traits::null_value()}
  {
  }

  template<typename U, typename P,
           detail::Requires<std::is_constructible<T, U&&>,
                            std::disjunction<std::is_same<std::decay_t<T>, bool>,
                                             std::negation<detail::constructs_or_converts_from_opt<T, U, P>>>> = true,
           detail::Requires<std::negation<std::is_convertible<U&&, T>>> = true>
  explicit constexpr opt(opt<U, P>&& other) : storage_{other.has_value() ? T{std::move(*other)} : traits::null_value()}
  {
  }

  template<typename U, typename P,
           detail::Requires<std::is_constructible<T, U&&>,
                            std::disjunction<std::is_same<std::decay_t<T>, bool>,
                                             std::negation<detail::constructs_or_converts_from_opt<T, U, P>>>> = true,
           detail::Requires<std::is_convertible<U&&, T>> = true>
  constexpr opt(opt<U, P>&& other) : storage_{other.has_value() ? T{std::move(*other)} : traits::null_value()}
  {
  }

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
    std::swap(storage_, other.storage_);
  }

  // observers
  constexpr const T* operator->() const    { return reinterpret_cast<const T*>(&storage_); }
  constexpr T* operator->()                { return reinterpret_cast<T*>(&storage_); }
  constexpr const T& operator*() const &   { return *reinterpret_cast<const T*>(&storage_); }
  constexpr T& operator*() &               { return *reinterpret_cast<T*>(&storage_); }
  constexpr T&& operator*() &&             { return std::move(*reinterpret_cast<T*>(&storage_)); }
  constexpr const T&& operator*() const && { return std::move(*reinterpret_cast<const T*>(&storage_)); }

  constexpr bool has_value() const noexcept(noexcept(traits::has_value(std::declval<T>())))
  {
    return traits::has_value(storage_);
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
  void reset() noexcept(noexcept(traits::null_value())) { storage_ = traits::null_value(); }
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
