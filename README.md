[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg?maxAge=3600)](https://raw.githubusercontent.com/mpusz/opt/master/LICENSE)
[![Travis CI](https://img.shields.io/travis/mpusz/opt/master.svg)](https://travis-ci.org/mpusz/opt)
[![Codecov](https://img.shields.io/codecov/c/github/mpusz/opt/master.svg)](https://codecov.io/github/mpusz/opt?branch=master)

# `opt<T, Policy>`

`opt<T, Policy>` is a class template designed to express optionality.

It has interface similar to `std::optional<T>` ([see here](http://en.cppreference.com/w/cpp/utility/optional))
but it does not use additional storage space for boolean flag needed to keep information if
the value is set or not. On the other hand type `T` should be a type that does not use all
values from its underlying representation thus leaving a possibly to use one of them as a _Null_
value that express emptiness of type `T`.

### `Policy` basic interface

`opt<T, Policy>` uses `Policy` class to provide all necessary information needed for proper class operation. The
simplest interface of `Policy` class for type `my_type` that uses `my_type_null_value` as _Null_ value contains
only one member function:
```cpp
struct my_opt_policy {
  static constexpr my_type null_value() noexcept { return my_type_null_value; }
};
opt<my_type, my_opt_policy> my_opt_val;
```
- `null_value()` member function returns (by value or reference) special value of type `my_type` that should be
   used by `opt<T, Policy>` as a special _Null_ value to mark emptiness.

`opt<T, Policy>` by default uses `opt_default_policy<T>` that can be specialized for user's type `T` so above example
may be refactored in the following way:
```cpp
template<>
struct opt_default_policy<my_type> {
  static constexpr my_type null_value() noexcept { return my_type_null_value; }
};
opt<my_type> my_opt_val;
```

The latter solution is suggested for user's types that will always have the same special value used as _Null_ while
the former should be used for builtin types like 'int' where the type can express different quantities thuse having
different _Null_ special values (e.g. age, price, month, etc).

### `Policy` types provided with the library

Beside already mentioned `opt_default_policy<T>` that can be specialized by the user for his/her type `my_type`,
the library provides 2 additional policy types:
```cpp
template<typename T, T NullValue>
struct opt_null_value_policy;
```
may be used to provide _Null_ value for integral type right in the class type definition. For example:
```cpp
opt<int, opt_null_value_policy<int, -1>> opt_int;
```

For types which values cannot be used as template argument (e.g. `float`) one can use:
```cpp
template<typename T, typename NullType>
struct opt_null_type_policy;
```
which assumes that `NullType::value` will represent _Null_ value. For example:
```cpp
  struct my_null_float { static constexpr float value = 0.0f; };
  opt<float, opt_null_type_policy<float, my_null_float>> opt_float;
```

### What if `my_type` does not provide equality comparison?

`opt<T, Policy>` needs to compare contained value with special _Null_ value provided by the _Policy_ type. By default
it tries to use `operator==()`. If the equality comparison  is not provided for `my_type` than either:
- `operator==()` should be provided, or
- additional `has_value(const T& value)` member function should be provided in the `Policy` type and it should return
  `false` if `value == null_value()` or `true` otherwise:
```cpp
struct my_opt_policy {
  static constexpr my_type null_value() noexcept { return my_type_null_value; }
  static constexpr bool has_value(const T& value) noexcept { return value != null_value(); }
};
opt<my_type, my_opt_policy> my_opt_val;
```

### What if `my_type` prevents me from setting value out of allowed range?

Sometimes `my_type` has some non-trivial logic or validation in its contructors or assignment operators that prevents
one to set special out-of-range value as _Null_ value for the type. In such case it is possible to extend `Policy`
class with following additional `storage_type` public member type. Such `storage_type` type:
- should be of the same size as `my_type` type
- should be constructible and assignable in the same way as `my_type`
- when storing not _Null_ value should have exactly the same memory representation as if the value was stored by
  `my_type` type
- should be used in `null_value()` and `has_value()` member functions instead of `my_type`. 

Below is an example of how `opt<bool>` could be implemented:
```cpp
template<>
struct opt_default_policy<bool> {
private:
  union storage {
    bool value;
    std::int8_t null_value = -1;
    storage() = default;
    constexpr storage(bool v) noexcept : value{v} {}
  };
public:
  using storage_type = storage;
  static constexpr storage_type null_value() noexcept      { return storage_type{}; }
  static constexpr bool has_value(storage_type s) noexcept { return s.null_value != -1; }
};
```

Here is another a bit more complicated example for some (not too smart ;-) ) `weekday` class:
```cpp
struct weekday {
  using underlying_type = std::int8_t;
  underlying_type value_;  // 0 - 6
  constexpr explicit weekday(underlying_type v) : value_{v}
  {
    if(v < 0 || v > 6) throw std::out_of_range{"weekday value outside of allowed range"};
  }
  constexpr weekday& operator=(underlying_type v)
  {
    if(v < 0 || v > 6) throw std::out_of_range{"weekday value outside of allowed range"};
    value_ = v;
    return *this;
  }
  underlying_type get() const { return value_; }
};
constexpr bool operator==(weekday lhs, weekday rhs) noexcept { return lhs.value_ == rhs.value_; }
constexpr bool operator==(weekday::underlying_type lhs, weekday rhs) noexcept { return lhs == rhs.value_; }
constexpr bool operator==(weekday lhs, weekday::underlying_type rhs) noexcept { return lhs.value_ == rhs; }

template<>
struct opt_default_policy<weekday> {
  union storage_type {
    weekday value;
    weekday::underlying_type null_value = std::numeric_limits<weekday::underlying_type>::max();

    constexpr storage_type() noexcept {};
    constexpr storage_type(weekday v) : value{v} {}
    constexpr storage_type(weekday::underlying_type v) : value{v} {}
    storage_type& operator=(weekday v)
    {
      value = v;
      return *this;
    }
  };

public:
  static storage_type null_value() noexcept { return storage_type{}; }
  static bool has_value(storage_type value) noexcept
  {
    return value.null_value != std::numeric_limits<weekday::underlying_type>::max();
  }
};
```
