/*
 * Copyright 2018 The StateChart Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef STATE_CHART_INTERNAL_FUNCTION_DISPATCHER_IMPL_H_
#define STATE_CHART_INTERNAL_FUNCTION_DISPATCHER_IMPL_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "absl/strings/str_join.h"
#include "absl/utility/utility.h"
#include "include/json/json.h"
#include "statechart/internal/function_dispatcher.h"
#include "statechart/internal/json_value_coder.h"
#include "statechart/internal/utility.h"
#include "statechart/platform/map_util.h"
#include "statechart/platform/types.h"

namespace state_chart {

namespace internal {

// Convert a vector of JSON value pointers to JSON strings formatted by
// Json::FastWriter.
std::vector<string> JsonValuesToStrings(
    const std::vector<const Json::Value*>& values);

// A base class that can be used to represent FunctionWrapper classes which can
// have different types based on their templates. This base class can be used as
// map::mapped_type to store FunctionWrappers in a map.
class BaseFunction {
 public:
  BaseFunction(const BaseFunction&) = delete;
  BaseFunction& operator=(const BaseFunction&) = delete;
  virtual ~BaseFunction() = default;

  virtual bool Execute(const std::vector<const Json::Value*>& inputs,
                       Json::Value* result) = 0;

  // Caller takes ownership.
  virtual BaseFunction* Clone() const = 0;

 protected:
  BaseFunction() = default;
};

template <typename Output, typename... Inputs>
class FunctionWrapper : public BaseFunction {
 public:
  typedef std::function<Output(Inputs...)> FunctionType;

  // Function object 'f' may not contain pointer types in arguments or
  // return type.
  explicit FunctionWrapper(FunctionType f) : f_(f) {}

  bool Execute(const std::vector<const Json::Value*>& inputs,
               Json::Value* result) override {
    // 'input_tuple' must contain non-const value types for output storage hence
    // 'const' and reference modifiers are removed.
    // E.g., If Inputs... is "const int, const string&, ..." then we want
    // input_tuple to be of type std::tuple<int, string>.
    std::tuple<typename std::remove_cv<
        typename std::remove_reference<Inputs>::type>::type...> input_tuple;
    if (!JsonValueListCoder::FromJson(inputs, &input_tuple)) {
      LOG(INFO) << "Cannot parse arguments: "
                << absl::StrJoin(JsonValuesToStrings(inputs), ", ");
      return false;
    }

    Output output = ::absl::apply(f_, input_tuple);
    return JsonValueCoder<Output>::ToJsonValue(output, result);
  }

  // Caller takes ownership.
  BaseFunction* Clone() const override {
    return new FunctionWrapper(*this);
  }

 protected:
  FunctionWrapper(const FunctionWrapper& other) : f_(other.f_) {}
  // Copy-move.
  FunctionWrapper& operator=(FunctionWrapper other) {
    f_ = std::move(other.f_);
    return *this;
  }

 private:
  FunctionType f_;
};

// Structs used to compute the type of a function object with operator().
// This struct deduces the type signature of the operator() and only works when
// operator() is not overloaded.
template <typename Functor>
struct FunctionTraits : public FunctionTraits<decltype(&Functor::operator())> {
};
// This struct computes the function type of a member function without the class
// type, i.e., as if the function is a free function.
template <typename ClassType, typename Output, typename... Inputs>
struct FunctionTraits<Output (ClassType::*)(Inputs...) const> {
  using Signature = Output(Inputs...);
};

}  // namespace internal

// A implementation for FunctionDispatcher.
// Sample use:
//  FunctionDispatcherImpl impl;
//  impl.RegisterFunction(
//      "my_function",
//      [](const string& str) -> string {
//        return StrCat(str, " world");
//      });
//  Json::Value in1("hello");
//  Json::Value out;
//  ASSERT_TRUE(impl.Execute("my_function", {&in1}, &out));
//  EXPECT_EQ(Json::Value("hello world"), out);
//
// Most of the std::functions should be supported.
// The input arguments must be of the form "T, const T or const T&" ,
// but not of the form "const T* or T*".
// TODO(srgandhe): Support const T* if possible.
class FunctionDispatcherImpl : public FunctionDispatcher {
 public:
  // The constructor adds builtin functions to the dispatcher.
  FunctionDispatcherImpl();
  ~FunctionDispatcherImpl() override = default;

  FunctionDispatcherImpl(const FunctionDispatcherImpl& other);

  // Copy-move.
  FunctionDispatcherImpl& operator=(FunctionDispatcherImpl other) {
    function_map_ = std::move(other.function_map_);
    return *this;
  }

  bool HasFunction(const string& function_name) const override;

  bool Execute(const string& function_name,
               const std::vector<const Json::Value*>& inputs,
               Json::Value* return_value) override;

  // Registers any std::function with a function_name.
  // Return true is registration is successful. Registering another function
  // with already registered function_name will ignore the new function and
  // return false.
  template <typename Output, typename... Inputs>
  bool RegisterFunction(const string& function_name,
                        std::function<Output(Inputs...)> function) {
    return function_map_
        .emplace(
            function_name,
            ::absl::WrapUnique(
                new internal::FunctionWrapper<Output, Inputs...>(function)))
        .second;
  }

  // Registers a function (pointer or object) that matches the function type
  // 'Signature'. This is primarily used to register function objects with
  // overloaded 'operator()' like the result of 'bind' expressions.
  // Return true if registration is successful, false otherwise.
  // Example usage:
  //   RegisterFunction<int(int)>("Increment", bind(plus<int>(), 1, _1));
  template <typename Signature, typename Function>
  bool RegisterFunction(const string& function_name, Function f) {
    static_assert(std::is_function<Signature>::value,
                  "Type 'Signature' must be a function type.");
    return RegisterFunction(function_name, std::function<Signature>{f});
  }

  // Registers a function pointer to a free (non-member) function.
  // Return true if registration is successful, false otherwise.
  template <typename Signature>
  bool RegisterFunction(const string& function_name, Signature* function) {
    return RegisterFunction<Signature, Signature*>(function_name, function);
  }

  // Registers a function object with non-overloaded operator(). This can be
  // used to directly register function objects returned by lambda expressions.
  // Return true if registration is successful, false otherwise.
  // Example usage:
  //   RegisterFunction("Increment", [](int i){ return i + 1; });
  template <typename Functor>
  bool RegisterFunction(const string& function_name, Functor f) {
    // Type deduction on unique operator().
    using Signature = typename internal::FunctionTraits<Functor>::Signature;
    return RegisterFunction<Signature, Functor>(function_name, f);
  }

 private:
  std::map<string, std::unique_ptr<internal::BaseFunction>> function_map_;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_FUNCTION_DISPATCHER_IMPL_H_
