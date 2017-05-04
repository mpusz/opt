[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg?maxAge=3600)](https://raw.githubusercontent.com/mpusz/opt/master/LICENSE)
[![Travis CI](https://img.shields.io/travis/mpusz/opt/master.svg?label=Travis%20CI)](https://travis-ci.org/mpusz/opt)
[![AppVeyor](https://img.shields.io/appveyor/ci/mpusz/opt/master.svg?label=AppVeyor)](https://ci.appveyor.com/project/mpusz/opt)
[![Codecov](https://img.shields.io/codecov/c/github/mpusz/opt/master.svg)](https://codecov.io/github/mpusz/opt?branch=master)

# `mp::opt<T, Policy>`

`mp::opt<T, Policy>` is a class template designed to express optionality.

It has interface similar to `std::optional<T>` ([see here](http://en.cppreference.com/w/cpp/utility/optional))
but it does not use additional storage space for boolean flag needed to keep information if
the value is set or not. On the other hand type `T` should be a type that does not use all
values from its underlying representation thus leaving a possibly to use one of them as a _Null_
value that express emptiness of type `T`.

## Dependencies

`mp::opt<T, Policy>` library depends on C++14 constexpr features and `std::optional<T>` existence so
a fairly modern compiler is needed. 

## Installation

`mp::opt<T, Policy>` is a header-only library. Its implementation was split to:
 - `opt.h` that contains the most important implementation and is intended to be included by the user with
   `#include <opt.h>` in the code
 - `opt_bits.h` that contains less important implementation details and is already included by `opt.h` header file

## Usage

Here is a simple example of how to use `mp::opt<T, Policy>` for some artificial type `price`:

```cpp
  using namespace mp;
  using price = int;
  using opt_price = opt<price, opt_null_value_policy<price, -1>>;
  static_assert(sizeof(opt_price) == sizeof(int));

  opt_price o1;
  assert(static_cast<bool>(o1) == false);
  assert(o1.has_value() == false);
  assert(o1.value_or(123) == 123);

  constexpr opt_price o2{99};
  assert(static_cast<bool>(o2) == true);
  assert(o2.has_value() == true);
  assert(*o2 == 99);
  assert(o2.value() == 99);
  assert(o2.value_or(123) == 99);

  opt_price o3{o2};
  assert(o3.has_value() == true);
  assert(*o3 == 99);

  o3 = o1;
  assert(o3.has_value() == false);

  o3 = o2;
  assert(o3.has_value() == true);
  assert(*o3 == 99);

  o3 = nullopt;
  assert(o3.has_value() == false);
```

### `Policy` basic interface

`mp::opt<T, Policy>` uses `Policy` class to provide all necessary information needed for proper class operation. The
simplest interface of `Policy` class for type `my_type` that uses `my_type_null_value` as _Null_ value contains
only one member function:
```cpp
struct my_opt_policy {
  static constexpr my_type null_value() noexcept { return my_type_null_value; }
};
mp::opt<my_type, my_opt_policy> my_opt_val;
```
- `null_value()` member function returns (by value or reference) special value of type `my_type` that should be
   used by `mp::opt<T, Policy>` as a special _Null_ value to mark emptiness.

`mp::opt<T, Policy>` by default uses `mp::opt_default_policy<T>` that can be specialized for user's type `T` so above
example may be refactored in the following way:
```cpp
namespace mp {
  template<>
  struct opt_default_policy<my_type> {
    static constexpr my_type null_value() noexcept { return my_type_null_value; }
  };
}
mp::opt<my_type> my_opt_val;
```

The latter solution is suggested for user's types that will always have the same special value used as _Null_ while
the former should be used for builtin types like 'int' where the type can express different quantities thus having
different _Null_ special values (e.g. age, price, month, etc).

### `Policy` types provided with the library

Beside already mentioned `mp::opt_default_policy<T>` that can be specialized by the user for his/her type `my_type`,
the library provides 2 additional policy types:
 - `mp::opt_null_value_policy<T, NullValue>`
   may be used to provide _Null_ value for integral type right in the class type definition. For example:
   ```cpp
   mp::opt<int, mp::opt_null_value_policy<int, -1>> opt_int;
   ```

 - `mp::opt_null_type_policy<T, NullType>` may be used for types which values cannot be used as template argument
   (e.g. `float`). That policy type assumes that `NullType::null_value` will represent _Null_ value. For example:
   ```cpp
   struct my_null_float { static constexpr float null_value = 0.0f; };
   mp::opt<float, mp::opt_null_type_policy<float, my_null_float>> opt_float;
   ```

### What if `my_type` does not provide equality comparison?

`mp::opt<T, Policy>` needs to compare contained value with special _Null_ value provided by the _Policy_ type. By default
it tries to use `operator==()`. If the equality comparison  is not provided for `my_type` than either:
- `operator==()` should be provided, or
- additional `has_value(const T& value)` member function should be provided in the `Policy` type and it should return
  `false` if `value == null_value()` or `true` otherwise:
```cpp
struct my_opt_policy {
  static constexpr my_type null_value() noexcept { return my_type_null_value; }
  static constexpr bool has_value(const T& value) noexcept { return value != null_value(); }
};
mp::opt<my_type, my_opt_policy> my_opt_val;
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
namepace mp {
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
}
```

Here is another a bit more complicated example for some (not too smart ;-) ) `weekday` class:
```cpp
class weekday {
public:
  using underlying_type = std::int8_t;
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
  constexpr underlying_type get() const noexcept { return value_; }
private:
  underlying_type value_;  // 0 - 6
};
constexpr bool operator==(weekday lhs, weekday rhs) noexcept { return lhs.get() == rhs.get(); }
constexpr bool operator==(weekday::underlying_type lhs, weekday rhs) noexcept { return lhs == rhs.get(); }
constexpr bool operator==(weekday lhs, weekday::underlying_type rhs) noexcept { return lhs.get() == rhs; }

namespace mp {
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
}
```
