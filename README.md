# `opt<T, Policy>`

`opt<T, Policy >` is a class template designed to express optionality.

It has interface similar to `std::optional<T>` ([see here](http://en.cppreference.com/w/cpp/utility/optional))
but it does not use additional storage space for boolean flag needed to keep information if
the value is set or not. On the other hand type `T` should be a type that does not use all
values from the underlying representation thus leaving a possibly to use one of them as MAGIC
a value that express emptiness of type `T`.

By default it uses `opt_default_policy<T>` that can be specialized for type `T` to provide information
which value is used to express emptiness. Policy class has a simple interface:

```
template<typename T>
struct MyOptPolicy {
  constexpr static bool is_initialized(const T& value) noexcept;
  constexpr static void set_empty(T& value) noexcept;
};
```

- `is_initialized(const T& value)` function is should return `false` if `value == MAGIC_VALUE` or `true`
otherwise,
- `set_empty(T& value)` function sets `value` to `MAGIC_VALUE`.

2 additional policy types are provided:
```
template<typename T, T MagicValue>
struct opt_magic_value_policy;
```
may be used to provide MAGIC value for integral type right in the class type definition. For example:
```
opt<int, opt_magic_value_policy<int, -1>> optInt;
```

For types which values cannot be used as template parameter (e.g. `float`) one can use
```
template<typename T, typename MagicType>
struct opt_magic_type_policy;
```
which assumes that `MagicType::value` will represent MAGIC value. For example:
```
  struct MyMagicFloat { static constexpr float value = 0.0f; };
  opt<float, opt_magic_type_policy<float, MyMagicFloat>> optFloat;
```
