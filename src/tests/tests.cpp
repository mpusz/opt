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
  class optTest : public ::testing::Test {
  public:
    using type = T;
    using traits = opt_traits<typename type::value_type>;
    const typename type::value_type value = traits::test_value;
    const typename type::value_type other_value = traits::other_value;
  };
  using test_types = ::testing::Types<opt<bool>, opt<int, opt_null_value_policy<int, -1>>, opt<float, opt_null_type_policy<float, NullFloating<float>>>>;
  TYPED_TEST_CASE(optTest, test_types);

}

TYPED_TEST(optTest, defaultConstructor)
{
  using opt_type = typename TestFixture::type;
  opt_type o;
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_THROW(o.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o.value_or(this->other_value));
}

TYPED_TEST(optTest, nulloptConstructor)
{
  using opt_type = typename TestFixture::type;
  opt_type o{nullopt};
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_THROW(o.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o.value_or(this->other_value));
}

TYPED_TEST(optTest, valueConstructor1)
{
  using opt_type = typename TestFixture::type;
  opt_type o{this->value};
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value, *o);
  EXPECT_EQ(this->value, o.value());
  EXPECT_EQ(this->value, o.value_or(this->other_value));
}

TYPED_TEST(optTest, valueConstructor2)
{
  using opt_type = typename TestFixture::type;
  opt_type o = this->value;
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value, *o);
  EXPECT_EQ(this->value, o.value());
  EXPECT_EQ(this->value, o.value_or(this->other_value));
}

TYPED_TEST(optTest, inPlaceConstructor)
{
  using opt_type = typename TestFixture::type;
  opt_type o{in_place, this->value};
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(this->value, *o);
  EXPECT_EQ(this->value, o.value());
  EXPECT_EQ(this->value, o.value_or(this->other_value));
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

TYPED_TEST(optTest, copyConstructionForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  auto o2{o1};
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTest, copyConstructionForNotEmpty)
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

TYPED_TEST(optTest, moveConstructionForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o1;
  auto o2{std::move(o1)};
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_THROW(o2.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o2.value_or(this->other_value));
}

TYPED_TEST(optTest, moveConstructionForNotEmpty)
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

TYPED_TEST(optTest, otherTypeCopyConstructionForEmpty)
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

TYPED_TEST(optTest, otherTypeCopyConstructionForNotEmpty)
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

TYPED_TEST(optTest, otherTypeMoveConstructionForEmpty)
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

TYPED_TEST(optTest, otherTypeMoveConstructionForNotEmpty)
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

TYPED_TEST(optTest, nullAssignmentEmptyForEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o;
  o = nullopt;
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_THROW(o.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o.value_or(this->other_value));
}

TYPED_TEST(optTest, nullAssignmentEmptyForNotEmpty)
{
  using opt_type = typename TestFixture::type;
  opt_type o{this->value};
  o = nullopt;
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_THROW(o.value(), bad_optional_access);
  EXPECT_EQ(this->other_value, o.value_or(this->other_value));
}

TYPED_TEST(optTest, copyAssignmentEmptyForEmpty)
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

TYPED_TEST(optTest, copyAssignmentNotEmptyForEmpty)
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

TYPED_TEST(optTest, copyAssignmentEmptyForNotEmpty)
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

TYPED_TEST(optTest, copyAssignmentNotEmptyForNotEmpty)
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

TYPED_TEST(optTest, moveAssignmentEmptyForEmpty)
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

TYPED_TEST(optTest, moveAssignmentNotEmptyForEmpty)
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

TYPED_TEST(optTest, moveAssignmentEmptyForNotEmpty)
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

TYPED_TEST(optTest, moveAssignmentNotEmptyForNotEmpty)
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

TYPED_TEST(optTest, valueAssignmentForEmpty)
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

TYPED_TEST(optTest, valueAssignmentForNotEmpty)
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

TYPED_TEST(optTest, swapEmptyWithEmpty)
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

TYPED_TEST(optTest, swapNotEmptyWithEmpty)
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

TYPED_TEST(optTest, swapEmptyWithNotEmpty)
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

TYPED_TEST(optTest, swapNotEmptyWithNotEmpty)
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
