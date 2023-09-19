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

#include <optional>
#include <type_traits>
#include <cassert>

namespace mp {

  template<typename T, typename Policy>
  class opt;

  namespace detail {

    template<typename... Args>
    using Requires = std::enable_if_t<std::conjunction<Args...>::value, bool>;

    // concepts needed by opt<T, Policy> class template
    template<typename T>
    struct is_opt : std::false_type {
    };
    template<typename T, typename P>
    struct is_opt<opt<T, P>> : std::true_type {
    };

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

    // detect if Policy::has_value() is present
    template<typename T, typename P>
    using has_value_t = decltype(std::declval<P>().has_value(std::declval<T>()));

    template<typename T, typename P, typename = std::void_t<>>
    struct has_has_value : std::false_type {
    };
    template<typename T, typename P>
    struct has_has_value<T, P, std::void_t<has_value_t<T, P>>> : std::is_same<has_value_t<T, P>, bool> {
    };

    // detect if Policy::storage_type is present
    template<typename T, typename Policy, typename = std::void_t<>>
    struct detect_storage_type {
      using type = T;
    };
    template<typename T, typename Policy>
    struct detect_storage_type<T, Policy, std::void_t<typename Policy::storage_type>> {
      using type = typename Policy::storage_type;
      static_assert(sizeof(T) == sizeof(type),
                    "'sizeof(Policy::storage_type) != sizeof(T)' consider using std::optional<T>");
    };

  }  // namespace detail
}
