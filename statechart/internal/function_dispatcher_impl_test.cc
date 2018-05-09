// Copyright 2018 The StateChart Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "statechart/internal/function_dispatcher_impl.h"

#include <gtest/gtest.h>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "include/json/json.h"

using ::std::placeholders::_1;
using ::std::placeholders::_2;

namespace state_chart {
namespace {

// Some class to demonstrate that a class's member function can be registered
// with FunctionDispatcherImpl.
class SomeClass {
 public:
  explicit SomeClass(const string& suffix) : suffix_(suffix) {}
  SomeClass(const SomeClass&) = delete;
  SomeClass& operator=(const SomeClass&) = delete;

  string Method0() const { return "0"; }
  string Method1(const string& foo) { return absl::StrCat(foo, " ", suffix_); }
  string Method2(const string& foo, const string& bar) {
    return absl::StrCat(foo, " ", bar, " ", suffix_);
  }

  static int StaticMethod1(int x) { return x; }

 private:
  const string suffix_;
};

class FunctionDispatcherImplTest : public testing::Test {
 protected:
  FunctionDispatcherImpl impl_;
};

TEST_F(FunctionDispatcherImplTest, HasFunction) {
  SomeClass a("");
  EXPECT_TRUE(impl_.RegisterFunction<string(const string&)>(
      "a.Method1", std::bind(&SomeClass::Method1, &a, _1)));
  EXPECT_TRUE(impl_.HasFunction("a.Method1"));

  // Adding another function for the same function_name will fail.
  EXPECT_FALSE(
      impl_.RegisterFunction("a.Method1", []() { return "lambda"; }));

  // Lambda registration capturing reference.
  EXPECT_TRUE(impl_.RegisterFunction(
      "a.Method1ByRef", [&a](const string& s) { return a.Method1(s); }));

  // Lambda registration capturing value.
  auto* a_ptr = &a;
  EXPECT_TRUE(impl_.RegisterFunction(
      "a.Method1ByValue",
      [a_ptr](const string& s) { return a_ptr->Method1(s); }));
}

TEST_F(FunctionDispatcherImplTest, ExecuteFunction) {
  SomeClass a("world");

  // Free function pointer registration.
  EXPECT_TRUE(impl_.RegisterFunction("SomeClass::StaticMethod1",
                                     &SomeClass::StaticMethod1));

  // Non-capturing lambda registration.
  EXPECT_TRUE(
      impl_.RegisterFunction("lambda", []() { return "lambda"; }));
  EXPECT_TRUE(
      impl_.RegisterFunction("lambda2", []() { return "lambda2"; }));

  // Lambda registration capturing reference.
  EXPECT_TRUE(impl_.RegisterFunction(
      "a.Method1", [&a](const string& s) { return a.Method1(s); }));

  // Bind expression result registration with explicit function signature.
  EXPECT_TRUE(impl_.RegisterFunction<string()>(
      "a.Method0", std::bind(&SomeClass::Method0, &a)));
  EXPECT_TRUE(impl_.RegisterFunction<string(const string&, const string&)>(
      "a.Method2", std::bind(&SomeClass::Method2, &a, _1, _2)));

  // The following will fail to compile since the input argument is a pointer.
  //  EXPECT_TRUE(impl.RegisterFunction(
  //      "lambda3",
  //      std::function<string(const string*)>([](const string* str) -> string {
  //        return "lambda3" + *str;
  //      })));

  {
    Json::Value return_value;
    EXPECT_TRUE(impl_.Execute("a.Method0", {}, &return_value));
    EXPECT_EQ(Json::Value("0"), return_value);
  }
  {
    Json::Value return_value;
    Json::Value input1("hello");
    EXPECT_TRUE(impl_.Execute("a.Method1", {&input1}, &return_value));
    EXPECT_EQ(Json::Value("hello world"), return_value);
  }
  {
    Json::Value return_value;
    Json::Value input1("hello");
    Json::Value input2("small");
    EXPECT_TRUE(impl_.Execute("a.Method2", {&input1, &input2}, &return_value));
    EXPECT_EQ(Json::Value("hello small world"), return_value);
  }
  {
    Json::Value return_value;
    EXPECT_TRUE(impl_.Execute("lambda", {}, &return_value));
    EXPECT_EQ(Json::Value("lambda"), return_value);
  }
  {
    Json::Value return_value;
    Json::Value input1(11);
    EXPECT_TRUE(
        impl_.Execute("SomeClass::StaticMethod1", {&input1}, &return_value));
    EXPECT_EQ(Json::Value(11), return_value);
  }
}

// Test that copying works.
TEST_F(FunctionDispatcherImplTest, CopyTest) {
  EXPECT_TRUE(impl_.RegisterFunction("Foo", []() { return "foo"; }));
  EXPECT_TRUE(impl_.RegisterFunction("Bar", []() { return "bar"; }));

  // Test copy constructor.
  {
    FunctionDispatcherImpl impl_copy(impl_);
    ASSERT_TRUE(impl_copy.HasFunction("Foo"));
    ASSERT_TRUE(impl_copy.HasFunction("Bar"));

    Json::Value return_value;
    EXPECT_TRUE(impl_copy.Execute("Foo", {}, &return_value));
    EXPECT_EQ(Json::Value("foo"), return_value);
    EXPECT_TRUE(impl_copy.Execute("Bar", {}, &return_value));
    EXPECT_EQ(Json::Value("bar"), return_value);
  }

  // Test assign operator.
  {
    FunctionDispatcherImpl impl_copy;
    impl_copy = impl_;
    ASSERT_TRUE(impl_copy.HasFunction("Foo"));
    ASSERT_TRUE(impl_copy.HasFunction("Bar"));

    Json::Value return_value;
    EXPECT_TRUE(impl_copy.Execute("Foo", {}, &return_value));
    EXPECT_EQ(Json::Value("foo"), return_value);
    EXPECT_TRUE(impl_copy.Execute("Bar", {}, &return_value));
    EXPECT_EQ(Json::Value("bar"), return_value);
  }
}

// Test that deleting the original after copying the function works.
TEST(FunctionDispatcherImpl, CopyThenDeleteOriginalTest) {
  auto impl = absl::make_unique<FunctionDispatcherImpl>();
  EXPECT_TRUE(impl->RegisterFunction("Foo", []() { return "foo"; }));
  EXPECT_TRUE(impl->RegisterFunction("Bar", []() { return "bar"; }));

  {
    // Test copy constructor.
    FunctionDispatcherImpl impl_copy(*impl);
    impl.reset();  // delete impl

    ASSERT_EQ(impl, nullptr);
    ASSERT_TRUE(impl_copy.HasFunction("Foo"));
    ASSERT_TRUE(impl_copy.HasFunction("Bar"));

    Json::Value return_value;
    EXPECT_TRUE(impl_copy.Execute("Foo", {}, &return_value));
    EXPECT_EQ(Json::Value("foo"), return_value);
    EXPECT_TRUE(impl_copy.Execute("Bar", {}, &return_value));
    EXPECT_EQ(Json::Value("bar"), return_value);

    // Test assign operator.
    impl.reset(new FunctionDispatcherImpl());
    *impl = impl_copy;
  }  // delete impl_copy

  ASSERT_TRUE(impl->HasFunction("Foo"));
  ASSERT_TRUE(impl->HasFunction("Bar"));

  Json::Value return_value;
  EXPECT_TRUE(impl->Execute("Foo", {}, &return_value));
  EXPECT_EQ(Json::Value("foo"), return_value);
  EXPECT_TRUE(impl->Execute("Bar", {}, &return_value));
  EXPECT_EQ(Json::Value("bar"), return_value);
}

}  // namespace
}  // namespace state_chart
