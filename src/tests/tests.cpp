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

  template<typename T, T MagicValue>
  using magic_opt = opt<T, opt_magic_value_policy<T, MagicValue>>;

  using namespace std::experimental;


  struct R {
    explicit R(double, double = 1.0) = delete;
    R(int num, int den = 1);
  };

  struct MyMagicFloat { static constexpr float value = 0.0f; };
  opt<float, opt_magic_type_policy<float, MyMagicFloat>> optFloat;
}

TEST(opt, defaultConstructor)
{
  magic_opt<int, -1> o;
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_EQ(999, o.value_or(999));
  EXPECT_THROW(o.value(), bad_optional_access);
}

TEST(opt, nulloptConstructor)
{
  magic_opt<int, -1> o{nullopt};
  EXPECT_FALSE(o);
  EXPECT_FALSE(o.has_value());
  EXPECT_EQ(999, o.value_or(999));
  EXPECT_THROW(o.value(), bad_optional_access);
}

TEST(opt, valueConstructor1)
{
  magic_opt<int, -1> o{123};
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(123, *o);
  EXPECT_EQ(123, o.value());
  EXPECT_EQ(123, o.value_or(999));
}

TEST(opt, valueConstructor2)
{
  magic_opt<int, -1> o = 123;
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(123, *o);
  EXPECT_EQ(123, o.value());
  EXPECT_EQ(123, o.value_or(999));
}

TEST(opt, inPlaceConstructor)
{
  magic_opt<int, -1> o{in_place, 123};
  EXPECT_TRUE(o);
  EXPECT_TRUE(o.has_value());
  EXPECT_EQ(123, *o);
  EXPECT_EQ(123, o.value());
  EXPECT_EQ(123, o.value_or(999));
}

//TEST(opt, inPlaceConstructorInitList)
//{
//  magic_opt<int, -1> o{in_place, {123}};
//  EXPECT_TRUE(o);
//  EXPECT_TRUE(o.has_value());
//  EXPECT_EQ(123, *o);
//  EXPECT_EQ(123, o.value());
//  EXPECT_EQ(123, o.value_or(999));
//}

TEST(opt, copyConstructionForEmpty)
{
  magic_opt<int, -1> o1;
  auto o2{o1};
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_EQ(999, o2.value_or(999));
  EXPECT_THROW(o2.value(), bad_optional_access);
}

TEST(opt, moveConstructionForEmpty)
{
  magic_opt<int, -1> o1;
  auto o2{std::move(o1)};
  EXPECT_FALSE(o2);
  EXPECT_FALSE(o2.has_value());
  EXPECT_EQ(999, o2.value_or(999));
  EXPECT_THROW(o2.value(), bad_optional_access);
}

