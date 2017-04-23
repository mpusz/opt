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

// explicit instantiation needed to make code coverage metrics work correctly
template class opt<int, opt_null_value_policy<int, -1>>;

namespace {

using namespace std::experimental;

struct MyBool {
  bool value_;
  MyBool() = default;
  constexpr MyBool(bool v) noexcept: value_{v} {}
  constexpr operator bool() const noexcept { return value_; }
};

struct R {
  explicit R(double, double = 1.0) = delete;
  R(int num, int den = 1);
};

}

template<>
struct opt_default_policy<bool> {
  using null_type = std::int8_t;
  static constexpr null_type null_value = -1;
  static bool has_value(bool value) noexcept { return reinterpret_cast<null_type&>(value) != null_value; }
  static void reset(bool& value) noexcept    { reinterpret_cast<null_type&>(value) = null_value; }
};

template<>
struct opt_default_policy<MyBool> {
  using null_type = std::int8_t;
  static constexpr null_type null_value = -1;
  static bool has_value(MyBool value) noexcept { return reinterpret_cast<null_type&>(value.value_) != null_value; }
  static void reset(MyBool& value) noexcept    { reinterpret_cast<null_type&>(value.value_) = null_value; }
};

namespace {

  template<typename T>
  struct NullFloating { static constexpr T value = 0.0f; };

  template<typename T>
  struct opt_traits;

  template<>
  struct opt_traits<bool> {
    using convertible_type = MyBool;
    using convertible_policy_type = opt_default_policy<convertible_type>;
    static constexpr bool test_value = true;
    static constexpr bool other_value = false;
  };

  template<>
  struct opt_traits<int> {
    using convertible_type = long;
    using convertible_policy_type = opt_null_value_policy<convertible_type, -1>;
    static constexpr int test_value = 123;
    static constexpr int other_value = 999;
  };

  template<>
  struct opt_traits<float> {
    using convertible_type = double;
    using convertible_policy_type = opt_null_type_policy<convertible_type, NullFloating<convertible_type>>;
    static constexpr float test_value = 3.14f;
    static constexpr float other_value = 123.456f;
  };

  template<typename T>
  class optTyped : public ::testing::Test {
  public:
    using type = T;
    using traits = opt_traits<typename type::value_type>;
    const typename type::value_type value = traits::test_value;
    const typename type::value_type other_value = traits::other_value;
  };
  using test_types = ::testing::Types<opt<bool>, opt<int, opt_null_value_policy<int, -1>>, opt<float, opt_null_type_policy<float, NullFloating<float>>>>;
  TYPED_TEST_CASE(optTyped, test_types);

}

TYPED_TEST(optTyped, defaultConstructor)
{
  using opt_type = typename TestFixture::type;
  opt_type o;
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_THROW(o.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o.value_or(this->other_value));

  const opt_type& co{o};
  EXPECT_FALSE(co);
  EXPECT_FALSE(co.has_value());
  EXPECT_THROW(co.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, co.value_or(this->other_value));
}

TYPED_TEST(optTyped, defaultConstructorRvalue)
{
  using opt_type = typename TestFixture::type;
  EXPECT_FALSE(opt_type{});
  EXPECT_FALSE(opt_type{}.has_value());
  EXPECT_THROW(opt_type{}.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, opt_type{}.value_or(this->other_value));

}

TYPED_TEST(optTyped, nulloptConstructor)
{
  using opt_type = typename TestFixture::type;
  opt_type o{nullopt};
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_THROW(o.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o.value_or(this->other_value));

  const opt_type& co{o};
  EXPECT_FALSE(co);
  EXPECT_FALSE(co.has_value());
  EXPECT_THROW(co.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, co.value_or(this->other_value));
}

TYPED_TEST(optTyped, valueConstructor1)
{
  using opt_type = typename TestFixture::type;
  opt_type o{this->value};
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value, *o);
  EXPECT_EQ(this->value, o.value());
  EXPECT_EQ(this->value, o.value_or(this->other_value));

  const opt_type& co{o};
  EXPECT_TRUE(co);
  EXPECT_TRUE(co.has_value());
  EXPECT_EQ(this->value, *co);
  EXPECT_EQ(this->value, co.value());
  EXPECT_EQ(this->value, co.value_or(this->other_value));
}

TYPED_TEST(optTyped, valueConstructor2)
{
  using opt_type = typename TestFixture::type;
  opt_type o = this->value;
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value, *o);
  EXPECT_EQ(this->value, o.value());
  EXPECT_EQ(this->value, o.value_or(this->other_value));

  const opt_type& co{o};
  EXPECT_TRUE(co);
  EXPECT_TRUE(co.has_value());
  EXPECT_EQ(this->value, *co);
  EXPECT_EQ(this->value, co.value());
  EXPECT_EQ(this->value, co.value_or(this->other_value));
}

TYPED_TEST(optTyped, valueConstructorRvalue)
{
  using opt_type = typename TestFixture::type;
  EXPECT_TRUE(opt_type{this->value});
  EXPECT_TRUE(opt_type{this->value}.has_value());
  EXPECT_EQ(this->value, *opt_type{this->value});
  EXPECT_EQ(this->value, opt_type{this->value}.value());
  EXPECT_EQ(this->value, opt_type{this->value}.value_or(this->other_value));
}

TYPED_TEST(optTyped, inPlaceConstructor)
{
  using opt_type = typename TestFixture::type;
  opt_type o{in_place, this->value};
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value, *o);
  EXPECT_EQ(this->value, o.value());
  EXPECT_EQ(this->value, o.value_or(this->other_value));

  const opt_type& co{o};
  EXPECT_TRUE(co);
  EXPECT_TRUE(co.has_value());
  EXPECT_EQ(this->value, *co);
  EXPECT_EQ(this->value, co.value());
  EXPECT_EQ(this->value, co.value_or(this->other_value));
}

//TEST(opt, inPlaceConstructorInitList)
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
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, copyConstructionForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value};
  auto o2{o1};
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, moveConstructionForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  auto o2{std::move(o1)};
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, moveConstructionForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value};
  auto o2{std::move(o1)};
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, otherTypeCopyConstructionForEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::convertible_type, typename TestFixture::traits::convertible_policy_type>;
  other_opt_type o1;
  opt_type o2{o1};
  EXPECT_FALSE(o1);
  EXPECT_FALSE(o1.has_value());
  EXPECT_THROW(o1.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o1.value_or(this->other_value));
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, otherTypeCopyConstructionForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::convertible_type, typename TestFixture::traits::convertible_policy_type>;
  other_opt_type o1{this->value};
  opt_type o2{o1};
  EXPECT_TRUE(o1);
  EXPECT_TRUE(o1.has_value());
  EXPECT_EQ(this->value, *o1);
  EXPECT_EQ(this->value, o1.value());
  EXPECT_EQ(this->value, o1.value_or(this->other_value));
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, otherTypeMoveConstructionForEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::convertible_type, typename TestFixture::traits::convertible_policy_type>;
  other_opt_type o1;
  opt_type o2{std::move(o1)};
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, otherTypeMoveConstructionForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::convertible_type, typename TestFixture::traits::convertible_policy_type>;
  other_opt_type o1{this->value};
  opt_type o2{std::move(o1)};
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, nullAssignmentEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o;
  o = nullopt;
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_THROW(o.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o.value_or(this->other_value));
}

TYPED_TEST(optTyped, nullAssignmentEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o{this->value};
  o = nullopt;
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_THROW(o.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o.value_or(this->other_value));
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
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, copyAssignmentNotEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value};
  opt_type o2;
  o2 = o1;
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, copyAssignmentEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  opt_type o2{this->value};
  o2 = o1;
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, copyAssignmentNotEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value};
  opt_type o2{this->other_value};
  o2 = o1;
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
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
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, moveAssignmentNotEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value};
  opt_type o2;
  o2 = std::move(o1);
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, moveAssignmentEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  opt_type o2{this->value};
  o2 = std::move(o1);
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, moveAssignmentNotEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value};
  opt_type o2{this->other_value};
  o2 = std::move(o1);
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, otherTypeCopyAssignmentEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::convertible_type, typename TestFixture::traits::convertible_policy_type>;
  other_opt_type o1;
  opt_type o2;
  o2 = o1;
  EXPECT_FALSE(o1);
  EXPECT_FALSE(o1.has_value());
  EXPECT_THROW(o1.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o1.value_or(this->other_value));
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, otherTypeCopyAssignmentNotEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::convertible_type, typename TestFixture::traits::convertible_policy_type>;
  other_opt_type o1{this->value};
  opt_type o2;
  o2 = o1;
  EXPECT_TRUE(o1);
  EXPECT_TRUE(o1.has_value());
  EXPECT_EQ(this->value, *o1);
  EXPECT_EQ(this->value, o1.value());
  EXPECT_EQ(this->value, o1.value_or(this->other_value));
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, otherTypeCopyAssignmentEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::convertible_type, typename TestFixture::traits::convertible_policy_type>;
  other_opt_type o1;
  opt_type o2{this->value};
  o2 = o1;
  EXPECT_FALSE(o1);
  EXPECT_FALSE(o1.has_value());
  EXPECT_THROW(o1.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o1.value_or(this->other_value));
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, otherTypeCopyAssignmentNotEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::convertible_type, typename TestFixture::traits::convertible_policy_type>;
  other_opt_type o1{this->value};
  opt_type o2{this->other_value};
  o2 = o1;
  EXPECT_TRUE(o1);
  EXPECT_TRUE(o1.has_value());
  EXPECT_EQ(this->value, *o1);
  EXPECT_EQ(this->value, o1.value());
  EXPECT_EQ(this->value, o1.value_or(this->other_value));
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, otherTypeMoveAssignmentEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::convertible_type, typename TestFixture::traits::convertible_policy_type>;
  other_opt_type o1;
  opt_type o2;
  o2 = std::move(o1);
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, otherTypeMoveAssignmentNotEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::convertible_type, typename TestFixture::traits::convertible_policy_type>;
  other_opt_type o1{this->value};
  opt_type o2;
  o2 = std::move(o1);
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, otherTypeMoveAssignmentEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::convertible_type, typename TestFixture::traits::convertible_policy_type>;
  other_opt_type o1;
  opt_type o2{this->value};
  o2 = std::move(o1);
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, otherTypeMoveAssignmentNotEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  using other_opt_type = opt<typename TestFixture::traits::convertible_type, typename TestFixture::traits::convertible_policy_type>;
  other_opt_type o1{this->value};
  opt_type o2{this->other_value};
  o2 = std::move(o1);
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, valueAssignmentForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o;
  o = typename TestFixture::traits::convertible_type{this->value};
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value, *o);
  EXPECT_EQ(this->value, o.value());
  EXPECT_EQ(this->value, o.value_or(this->other_value));
}

TYPED_TEST(optTyped, valueAssignmentForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o{this->other_value};
  o = typename TestFixture::traits::convertible_type{this->value};
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value, *o);
  EXPECT_EQ(this->value, o.value());
  EXPECT_EQ(this->value, o.value_or(this->other_value));
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
  EXPECT_EQ(this->other_value, o1.value_or(this->other_value));
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, swapNotEmptyWithEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value};
  opt_type o2;
  std::swap(o1, o2);
  EXPECT_FALSE(o1);
  EXPECT_FALSE(o1.has_value());
  EXPECT_THROW(o1.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o1.value_or(this->other_value));
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, swapEmptyWithNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  opt_type o2{this->value};
  std::swap(o1, o2);
  EXPECT_TRUE(o1);
  EXPECT_TRUE(o1.has_value());
  EXPECT_EQ(this->value, *o1);
  EXPECT_EQ(this->value, o1.value());
  EXPECT_EQ(this->value, o1.value_or(this->other_value));
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTyped, swapNotEmptyWithNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1{this->value};
  opt_type o2{this->other_value};
  std::swap(o1, o2);
  EXPECT_TRUE(o1);
  EXPECT_TRUE(o1.has_value());
  EXPECT_EQ(this->other_value, *o1);
  EXPECT_EQ(this->other_value, o1.value());
  EXPECT_EQ(this->other_value, o1.value_or(this->value));
  EXPECT_TRUE(o2);
  EXPECT_TRUE(o2.has_value());
  EXPECT_EQ(this->value, *o2);
  EXPECT_EQ(this->value, o2.value());
  EXPECT_EQ(this->value, o2.value_or(this->other_value));
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
