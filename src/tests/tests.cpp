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

#include "opt.h"
#include <gtest/gtest.h>

namespace {

  using namespace std::experimental;

  struct my_bool {
    std::uint8_t value_;
    my_bool() = default;
    constexpr my_bool(bool v) noexcept : value_{v} {}
    explicit constexpr my_bool(std::uint8_t v) noexcept : value_{v} {}
    constexpr operator bool() const noexcept { return value_; }
  };

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
}

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
  static constexpr storage_type null_value() noexcept { return storage_type{}; }
  static constexpr bool has_value(storage_type s) noexcept { return s.null_value != -1; }
};

template<>
struct opt_default_policy<my_bool> {
  static my_bool null_value() noexcept { return my_bool{std::uint8_t{255}}; }
  static bool has_value(my_bool value) noexcept { return value.value_ != 255; }
};

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

namespace {

  template<typename T>
  struct null_floating {
    static constexpr T value = 0.0f;
  };

  template<typename T>
  struct opt_traits;

  template<>
  struct opt_traits<bool> {
    static_assert(std::is_constructible<bool, my_bool&&>::value);
    static_assert(std::is_convertible<my_bool&&, bool>::value);

    static constexpr bool value_1 = true;
    static constexpr bool value_2 = false;

    using other_type = my_bool;
    using other_policy_type = opt_default_policy<other_type>;
    static const other_type other_value_1;
    static const other_type other_value_2;
  };
  constexpr opt_traits<bool>::other_type opt_traits<bool>::other_value_1 = true;
  constexpr opt_traits<bool>::other_type opt_traits<bool>::other_value_2 = false;

  template<>
  struct opt_traits<long> {
    static_assert(std::is_constructible<long, int&&>::value);
    static_assert(std::is_convertible<int&&, long>::value);

    static constexpr long value_1 = 123;
    static constexpr long value_2 = 999;

    using other_type = int;
    using other_policy_type = opt_null_value_policy<other_type, std::numeric_limits<other_type>::max()>;
    static constexpr other_type other_value_1 = 123;
    static constexpr other_type other_value_2 = 999;
  };

  template<>
  struct opt_traits<weekday> {
    static_assert(std::is_constructible<weekday, std::int8_t&&>::value);
    static_assert(!std::is_convertible<std::int8_t&&, weekday>::value);

    static const weekday value_1;
    static const weekday value_2;

    using other_type = decltype(std::declval<weekday>().value_);
    using other_policy_type = opt_null_value_policy<other_type, -1>;
    static constexpr other_type other_value_1 = 0;
    static constexpr other_type other_value_2 = 3;
  };
  constexpr weekday opt_traits<weekday>::value_1{0};
  constexpr weekday opt_traits<weekday>::value_2{3};

  template<>
  struct opt_traits<double> {
    static_assert(std::is_constructible<double, float&&>::value);
    static_assert(std::is_convertible<float&&, double>::value);

    static constexpr double value_1 = 3.14f;
    static constexpr double value_2 = 123.456f;

    using other_type = float;
    using other_policy_type = opt_null_type_policy<other_type, null_floating<other_type>>;
    static constexpr other_type other_value_1 = 3.14f;
    static constexpr other_type other_value_2 = 123.456f;
  };

  template<typename T>
  class optTyped : public ::testing::Test {
  public:
    using type = T;
    using traits = opt_traits<typename type::value_type>;
    const typename type::value_type value_1 = traits::value_1;
    const typename type::value_type value_2 = traits::value_2;
    const typename traits::other_type other_value_1 = traits::other_value_1;
    const typename traits::other_type other_value_2 = traits::other_value_2;
  };
  using test_types = ::testing::Types<opt<bool>, opt<weekday>, opt<long, opt_null_value_policy<long, -1>>,
                                      opt<double, opt_null_type_policy<double, null_floating<double>>>>;
  TYPED_TEST_CASE(optTyped, test_types);
}

TYPED_TEST(optTyped, defaultConstructor)
{
  using opt_type = typename TestFixture::type;
  opt_type o;
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_THROW(o.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o.value_or(this->value_2));

  const opt_type& co{o};
  EXPECT_FALSE(co);
  EXPECT_FALSE(co.has_value());
  EXPECT_THROW(co.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, co.value_or(this->value_2));
}

TYPED_TEST(optTyped, defaultConstructorRvalue)
{
  using opt_type = typename TestFixture::type;
  auto make = [] { return opt_type{}; };
  EXPECT_FALSE(make());
  EXPECT_FALSE(make().has_value());
  EXPECT_THROW(make().value(), bad_optional_access);
  EXPECT_EQ(this->value_2, make().value_or(this->value_2));
}

TYPED_TEST(optTyped, defaultConstructorConstRvalue)
{
  using opt_type = typename TestFixture::type;
  auto make = []() -> const opt_type { return opt_type{}; };
  EXPECT_FALSE(make());
  EXPECT_FALSE(make().has_value());
  EXPECT_THROW(make().value(), bad_optional_access);
  EXPECT_EQ(this->value_2, make().value_or(this->value_2));
}

TYPED_TEST(optTyped, nulloptConstructor)
{
  using opt_type = typename TestFixture::type;
  opt_type o{nullopt};
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_THROW(o.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o.value_or(this->value_2));

  const opt_type& co{o};
  EXPECT_FALSE(co);
  EXPECT_FALSE(co.has_value());
  EXPECT_THROW(co.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, co.value_or(this->value_2));
}

TYPED_TEST(optTyped, valueConstructor1)
{
  using opt_type = typename TestFixture::type;
  opt_type o{this->value_1};
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value_1, *o);
  EXPECT_EQ(this->value_1, o.value());
  EXPECT_EQ(this->value_1, o.value_or(this->value_2));

  const opt_type& co{o};
  EXPECT_TRUE(co);
  EXPECT_TRUE(co.has_value());
  EXPECT_EQ(this->value_1, *co);
  EXPECT_EQ(this->value_1, co.value());
  EXPECT_EQ(this->value_1, co.value_or(this->value_2));
}

TYPED_TEST(optTyped, valueConstructor2)
{
  using opt_type = typename TestFixture::type;
  opt_type o = this->value_1;
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value_1, *o);
  EXPECT_EQ(this->value_1, o.value());
  EXPECT_EQ(this->value_1, o.value_or(this->value_2));

  const opt_type& co{o};
  EXPECT_TRUE(co);
  EXPECT_TRUE(co.has_value());
  EXPECT_EQ(this->value_1, *co);
  EXPECT_EQ(this->value_1, co.value());
  EXPECT_EQ(this->value_1, co.value_or(this->value_2));
}

TYPED_TEST(optTyped, valueConstructorOther1)
{
  using opt_type = typename TestFixture::type;
  opt_type o{this->other_value_1};
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value_1, *o);
  EXPECT_EQ(this->value_1, o.value());
  EXPECT_EQ(this->value_1, o.value_or(this->value_2));

  const opt_type& co{o};
  EXPECT_TRUE(co);
  EXPECT_TRUE(co.has_value());
  EXPECT_EQ(this->value_1, *co);
  EXPECT_EQ(this->value_1, co.value());
  EXPECT_EQ(this->value_1, co.value_or(this->value_2));
}

TYPED_TEST(optTyped, valueConstructorRvalue)
{
  using opt_type = typename TestFixture::type;
  auto make = [&] { return opt_type{this->value_1}; };
  EXPECT_TRUE(make());
  EXPECT_TRUE(make().has_value());
  EXPECT_EQ(this->value_1, *make());
  EXPECT_EQ(this->value_1, make().value());
  EXPECT_EQ(this->value_1, make().value_or(this->value_2));
}

TYPED_TEST(optTyped, valueConstructorConstRvalue)
{
  using opt_type = typename TestFixture::type;
  auto make = [&]() -> const opt_type { return opt_type{this->value_1}; };
  EXPECT_TRUE(make());
  EXPECT_TRUE(make().has_value());
  EXPECT_EQ(this->value_1, *make());
  EXPECT_EQ(this->value_1, make().value());
  EXPECT_EQ(this->value_1, make().value_or(this->value_2));
}

TYPED_TEST(optTyped, inPlaceConstructor)
{
  using opt_type = typename TestFixture::type;
  opt_type o{in_place, this->value_1};
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value_1, *o);
  EXPECT_EQ(this->value_1, o.value());
  EXPECT_EQ(this->value_1, o.value_or(this->value_2));

  const opt_type& co{o};
  EXPECT_TRUE(co);
  EXPECT_TRUE(co.has_value());
  EXPECT_EQ(this->value_1, *co);
  EXPECT_EQ(this->value_1, co.value());
  EXPECT_EQ(this->value_1, co.value_or(this->value_2));
}

// TEST(opt, inPlaceConstructorInitList)
//{
//  null_value_opt<int, -1> o{in_place, {123}};
//  EXPECT_TRUE(o);
//  EXPECT_TRUE(o.has_value());
//  EXPECT_EQ(123, *o);
//  EXPECT_EQ(123, o.value());
//  EXPECT_EQ(123, o.value_or(999));
//}

TYPED_TEST(optTyped, copyConstructionForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  auto o2{o1};
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, copyConstructionForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value_1};
  auto o2{o1};
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, moveConstructionForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  auto o2{std::move(o1)};
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, moveConstructionForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value_1};
  auto o2{std::move(o1)};
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, otherTypeCopyConstructionForEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::other_type, typename TestFixture::traits::other_policy_type>;
  other_opt_type o1;
  opt_type o2{o1};
  EXPECT_FALSE(o1);
  EXPECT_FALSE(o1.has_value());
  EXPECT_THROW(o1.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o1.value_or(this->other_value_2));
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, otherTypeCopyConstructionForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::other_type, typename TestFixture::traits::other_policy_type>;
  other_opt_type o1{this->other_value_1};
  opt_type o2{o1};
  EXPECT_TRUE(o1);
  EXPECT_TRUE(o1.has_value());
  EXPECT_EQ(this->value_1, *o1);
  EXPECT_EQ(this->value_1, o1.value());
  EXPECT_EQ(this->value_1, o1.value_or(this->other_value_2));
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, otherTypeMoveConstructionForEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::other_type, typename TestFixture::traits::other_policy_type>;
  other_opt_type o1;
  opt_type o2{std::move(o1)};
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, otherTypeMoveConstructionForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::other_type, typename TestFixture::traits::other_policy_type>;
  other_opt_type o1{this->other_value_1};
  opt_type o2{std::move(o1)};
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, nullAssignmentEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o;
  o = nullopt;
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_THROW(o.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o.value_or(this->value_2));
}

TYPED_TEST(optTyped, nullAssignmentEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o{this->value_1};
  o = nullopt;
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_THROW(o.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o.value_or(this->value_2));
}

TYPED_TEST(optTyped, copyAssignmentEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  opt_type o2;
  o2 = o1;
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, copyAssignmentNotEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value_1};
  opt_type o2;
  o2 = o1;
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, copyAssignmentEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  opt_type o2{this->value_1};
  o2 = o1;
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, copyAssignmentNotEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value_1};
  opt_type o2{this->value_2};
  o2 = o1;
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, moveAssignmentEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  opt_type o2;
  o2 = std::move(o1);
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, moveAssignmentNotEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value_1};
  opt_type o2;
  o2 = std::move(o1);
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, moveAssignmentEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  opt_type o2{this->value_1};
  o2 = std::move(o1);
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, moveAssignmentNotEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value_1};
  opt_type o2{this->value_2};
  o2 = std::move(o1);
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, otherTypeCopyAssignmentEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::other_type, typename TestFixture::traits::other_policy_type>;
  other_opt_type o1;
  opt_type o2;
  o2 = o1;
  EXPECT_FALSE(o1);
  EXPECT_FALSE(o1.has_value());
  EXPECT_THROW(o1.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o1.value_or(this->other_value_2));
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, otherTypeCopyAssignmentNotEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::other_type, typename TestFixture::traits::other_policy_type>;
  other_opt_type o1{this->other_value_1};
  opt_type o2;
  o2 = o1;
  EXPECT_TRUE(o1);
  EXPECT_TRUE(o1.has_value());
  EXPECT_EQ(this->value_1, *o1);
  EXPECT_EQ(this->value_1, o1.value());
  EXPECT_EQ(this->value_1, o1.value_or(this->other_value_2));
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, otherTypeCopyAssignmentEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::other_type, typename TestFixture::traits::other_policy_type>;
  other_opt_type o1;
  opt_type o2{this->value_1};
  o2 = o1;
  EXPECT_FALSE(o1);
  EXPECT_FALSE(o1.has_value());
  EXPECT_THROW(o1.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o1.value_or(this->other_value_2));
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, otherTypeCopyAssignmentNotEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::other_type, typename TestFixture::traits::other_policy_type>;
  other_opt_type o1{this->other_value_1};
  opt_type o2{this->value_2};
  o2 = o1;
  EXPECT_TRUE(o1);
  EXPECT_TRUE(o1.has_value());
  EXPECT_EQ(this->value_1, *o1);
  EXPECT_EQ(this->value_1, o1.value());
  EXPECT_EQ(this->value_1, o1.value_or(this->other_value_2));
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, otherTypeMoveAssignmentEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::other_type, typename TestFixture::traits::other_policy_type>;
  other_opt_type o1;
  opt_type o2;
  o2 = std::move(o1);
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, otherTypeMoveAssignmentNotEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::other_type, typename TestFixture::traits::other_policy_type>;
  other_opt_type o1{this->other_value_1};
  opt_type o2;
  o2 = std::move(o1);
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, otherTypeMoveAssignmentEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::other_type, typename TestFixture::traits::other_policy_type>;
  other_opt_type o1;
  opt_type o2{this->value_1};
  o2 = std::move(o1);
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, otherTypeMoveAssignmentNotEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::other_type, typename TestFixture::traits::other_policy_type>;
  other_opt_type o1{this->other_value_1};
  opt_type o2{this->value_2};
  o2 = std::move(o1);
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, valueAssignmentForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o;
  o = this->value_1;
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value_1, *o);
  EXPECT_EQ(this->value_1, o.value());
  EXPECT_EQ(this->value_1, o.value_or(this->value_2));
}

TYPED_TEST(optTyped, valueAssignmentForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o{this->value_2};
  o = this->value_1;
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value_1, *o);
  EXPECT_EQ(this->value_1, o.value());
  EXPECT_EQ(this->value_1, o.value_or(this->value_2));
}

TYPED_TEST(optTyped, swapEmptyWithEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  opt_type o2;
  std::swap(o1, o2);
  EXPECT_FALSE(o1);
  EXPECT_FALSE(o1.has_value());
  EXPECT_THROW(o1.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o1.value_or(this->value_2));
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, swapNotEmptyWithEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value_1};
  opt_type o2;
  std::swap(o1, o2);
  EXPECT_FALSE(o1);
  EXPECT_FALSE(o1.has_value());
  EXPECT_THROW(o1.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o1.value_or(this->value_2));
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, swapEmptyWithNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  opt_type o2{this->value_1};
  std::swap(o1, o2);
  EXPECT_TRUE(o1);
  EXPECT_TRUE(o1.has_value());
  EXPECT_EQ(this->value_1, *o1);
  EXPECT_EQ(this->value_1, o1.value());
  EXPECT_EQ(this->value_1, o1.value_or(this->value_2));
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->value_2, o2.value_or(this->value_2));
}

TYPED_TEST(optTyped, swapNotEmptyWithNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value_1};
  opt_type o2{this->value_2};
  std::swap(o1, o2);
  EXPECT_TRUE(o1);
  EXPECT_TRUE(o1.has_value());
  EXPECT_EQ(this->value_2, *o1);
  EXPECT_EQ(this->value_2, o1.value());
  EXPECT_EQ(this->value_2, o1.value_or(this->value_1));
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value_1, *o2);
  EXPECT_EQ(this->value_1, o2.value());
  EXPECT_EQ(this->value_1, o2.value_or(this->value_2));
}

TEST(opt, constructOutOfRange)
{
  weekday::underlying_type d1{7};
  EXPECT_THROW(opt<weekday>{d1}, std::out_of_range);
  weekday::underlying_type d2{-1};
  EXPECT_THROW(opt<weekday>{d2}, std::out_of_range);
}

TEST(opt, assignOutOfRange)
{
  weekday::underlying_type d1{7};
  weekday::underlying_type d2{-1};
  opt<weekday> o;
  EXPECT_THROW(o = d1, std::out_of_range);
  EXPECT_THROW(o = d2, std::out_of_range);
}

TEST(opt, dereferenceOperator)
{
  weekday::underlying_type d{1};
  opt<weekday> w{d};
  EXPECT_TRUE(w);
  EXPECT_TRUE(w.has_value());
  EXPECT_EQ(d, *w);
  EXPECT_EQ(d, w.value());
  EXPECT_EQ(d, w->get());
  EXPECT_EQ(d, w.value_or(weekday::underlying_type{3}));
}

TEST(opt, dereferenceOperatorRvalue)
{
  weekday::underlying_type d{1};
  auto make = [&] { return opt<weekday>{d}; };
  EXPECT_TRUE(make());
  EXPECT_TRUE(make().has_value());
  EXPECT_EQ(d, *make());
  EXPECT_EQ(d, make().value());
  EXPECT_EQ(d, make()->get());
  EXPECT_EQ(d, make().value_or(weekday::underlying_type{3}));
}

TEST(opt, dereferenceOperatorConstRvalue)
{
  weekday::underlying_type d{1};
  auto make = [&]() -> const opt<weekday> { return opt<weekday>{d}; };
  EXPECT_TRUE(make());
  EXPECT_TRUE(make().has_value());
  EXPECT_EQ(d, *make());
  EXPECT_EQ(d, make().value());
  EXPECT_EQ(d, make()->get());
  EXPECT_EQ(d, make().value_or(weekday::underlying_type{3}));
}

TEST(optCompare, bothNotEmptyEqual)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i1{1}, i2{1};
  EXPECT_TRUE(i1 == i2);
  EXPECT_FALSE(i1 != i2);
  EXPECT_FALSE(i1 < i2);
  EXPECT_FALSE(i1 > i2);
  EXPECT_TRUE(i1 <= i2);
  EXPECT_TRUE(i1 >= i2);
}

TEST(optCompare, bothNotEmptyLess)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i1{1}, i2{2};
  EXPECT_FALSE(i1 == i2);
  EXPECT_TRUE(i1 != i2);
  EXPECT_TRUE(i1 < i2);
  EXPECT_FALSE(i1 > i2);
  EXPECT_TRUE(i1 <= i2);
  EXPECT_FALSE(i1 >= i2);
}

TEST(optCompare, bothNotEmptyGreater)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i1{2}, i2{1};
  EXPECT_FALSE(i1 == i2);
  EXPECT_TRUE(i1 != i2);
  EXPECT_FALSE(i1 < i2);
  EXPECT_TRUE(i1 > i2);
  EXPECT_FALSE(i1 <= i2);
  EXPECT_TRUE(i1 >= i2);
}

TEST(optCompare, bothEmpty)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i1, i2;
  EXPECT_TRUE(i1 == i2);
  EXPECT_FALSE(i1 != i2);
  EXPECT_FALSE(i1 < i2);
  EXPECT_FALSE(i1 > i2);
  EXPECT_TRUE(i1 <= i2);
  EXPECT_TRUE(i1 >= i2);
}

TEST(optCompare, emptyNotEmpty)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i1, i2{1};
  EXPECT_FALSE(i1 == i2);
  EXPECT_TRUE(i1 != i2);
  EXPECT_TRUE(i1 < i2);
  EXPECT_FALSE(i1 > i2);
  EXPECT_TRUE(i1 <= i2);
  EXPECT_FALSE(i1 >= i2);
}

TEST(optCompare, notEmptyEmpty)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i1{1}, i2;
  EXPECT_FALSE(i1 == i2);
  EXPECT_TRUE(i1 != i2);
  EXPECT_FALSE(i1 < i2);
  EXPECT_TRUE(i1 > i2);
  EXPECT_FALSE(i1 <= i2);
  EXPECT_TRUE(i1 >= i2);
}

TEST(optCompare, optNotEmptyValueEqual)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i{1};
  EXPECT_TRUE(i == 1);
  EXPECT_FALSE(i != 1);
  EXPECT_FALSE(i < 1);
  EXPECT_FALSE(i > 1);
  EXPECT_TRUE(i <= 1);
  EXPECT_TRUE(i >= 1);
}

TEST(optCompare, optNotEmptyValueLess)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i{1};
  EXPECT_FALSE(i == 2);
  EXPECT_TRUE(i != 2);
  EXPECT_TRUE(i < 2);
  EXPECT_FALSE(i > 2);
  EXPECT_TRUE(i <= 2);
  EXPECT_FALSE(i >= 2);
}

TEST(optCompare, optNotEmptyValueGreater)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i{2};
  EXPECT_FALSE(i == 1);
  EXPECT_TRUE(i != 1);
  EXPECT_FALSE(i < 1);
  EXPECT_TRUE(i > 1);
  EXPECT_FALSE(i <= 1);
  EXPECT_TRUE(i >= 1);
}

TEST(optCompare, optEmptyValue)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i;
  EXPECT_FALSE(i == 1);
  EXPECT_TRUE(i != 1);
  EXPECT_TRUE(i < 1);
  EXPECT_FALSE(i > 1);
  EXPECT_TRUE(i <= 1);
  EXPECT_FALSE(i >= 1);
}

TEST(optCompare, valueOptNotEmptyEqual)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i{1};
  EXPECT_TRUE(1 == i);
  EXPECT_FALSE(1 != i);
  EXPECT_FALSE(1 < i);
  EXPECT_FALSE(1 > i);
  EXPECT_TRUE(1 <= i);
  EXPECT_TRUE(1 >= i);
}

TEST(optCompare, valueOptNotEmptyLess)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i{2};
  EXPECT_FALSE(1 == i);
  EXPECT_TRUE(1 != i);
  EXPECT_TRUE(1 < i);
  EXPECT_FALSE(1 > i);
  EXPECT_TRUE(1 <= i);
  EXPECT_FALSE(1 >= i);
}

TEST(optCompare, valueOptNotEmptyGreater)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i{1};
  EXPECT_FALSE(2 == i);
  EXPECT_TRUE(2 != i);
  EXPECT_FALSE(2 < i);
  EXPECT_TRUE(2 > i);
  EXPECT_FALSE(2 <= i);
  EXPECT_TRUE(2 >= i);
}

TEST(optCompare, valueOptEmpty)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i;
  EXPECT_FALSE(1 == i);
  EXPECT_TRUE(1 != i);
  EXPECT_FALSE(1 < i);
  EXPECT_TRUE(1 > i);
  EXPECT_FALSE(1 <= i);
  EXPECT_TRUE(1 >= i);
}

TEST(optCompare, optEmptyNullopt)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i;
  EXPECT_TRUE(i == nullopt);
  EXPECT_FALSE(i != nullopt);
  EXPECT_FALSE(i < nullopt);
  EXPECT_FALSE(i > nullopt);
  EXPECT_TRUE(i <= nullopt);
  EXPECT_TRUE(i >= nullopt);
}

TEST(optCompare, optNotEmptyNullopt)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i{1};
  EXPECT_FALSE(i == nullopt);
  EXPECT_TRUE(i != nullopt);
  EXPECT_FALSE(i < nullopt);
  EXPECT_TRUE(i > nullopt);
  EXPECT_FALSE(i <= nullopt);
  EXPECT_TRUE(i >= nullopt);
}

TEST(optCompare, nulloptOptEmpty)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i;
  EXPECT_TRUE(nullopt == i);
  EXPECT_FALSE(nullopt != i);
  EXPECT_FALSE(nullopt < i);
  EXPECT_FALSE(nullopt > i);
  EXPECT_TRUE(nullopt <= i);
  EXPECT_TRUE(nullopt >= i);
}

TEST(optCompare, nulloptOptNotEmpty)
{
  using opt_int = opt<int, opt_null_value_policy<int, -1>>;
  opt_int i{1};
  EXPECT_FALSE(nullopt == i);
  EXPECT_TRUE(nullopt != i);
  EXPECT_TRUE(nullopt < i);
  EXPECT_FALSE(nullopt > i);
  EXPECT_TRUE(nullopt <= i);
  EXPECT_FALSE(nullopt >= i);
}
