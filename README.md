# `opt<T, Policy>`

`opt<T, Policy >` is a class template designed to express optionality.

It has interface similar to `std::optional<T>` ([see here](http://en.cppreference.com/w/cpp/utility/optional))
but it does not use additional storage space for boolean flag needed to keep information if
the value is set or not. On the other hand type `T` should be a type that does not use all
values from its underlying representation thus leaving a possibly to use one of them as a _Null_
value that express emptiness of type `T`.

By default it uses `opt_default_policy<T>` that can be specialized for type `T` to provide information
which value is used to express emptiness.

Policy class has a simple interface:

```
template<typename T>
struct MyOptPolicy {
  constexpr static bool has_value(const T& value) noexcept;
  constexpr static void reset(T& value) noexcept;
};
```

- `has_value(const T& value)` function is should return `false` if `value == NullValue` or `true`
otherwise,
- `reset(T& value)` function sets `value` to `NullValue`.

2 additional policy types are provided:
```
template<typename T, T NullValue>
struct opt_null_value_policy;
```
may be used to provide _Null_ value for integral type right in the class type definition. For example:
```
opt<int, opt_null_value_policy<int, -1>> optInt;
```

For types which values cannot be used as template parameter (e.g. `float`) one can use
```
template<typename T, typename NullType>
struct opt_null_type_policy;
```
which assumes that `NullType::value` will represent _Null_ value. For example:
```
  struct MyNullFloat { static constexpr float value = 0.0f; };
  opt<float, opt_null_type_policy<float, MyNullFloat>> optFloat;
```
