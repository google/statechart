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

#ifndef STATE_CHART_INTERNAL_LIGHT_WEIGHT_DATAMODEL_H_
#define STATE_CHART_INTERNAL_LIGHT_WEIGHT_DATAMODEL_H_

#include <list>
#include <map>
#include <memory>
#include <string>

#include <glog/logging.h>

#include "absl/strings/str_cat.h"
#include "include/json/json.h"
#include "statechart/internal/datamodel.h"
#include "statechart/internal/runtime.h"
#include "statechart/logging.h"
#include "statechart/platform/str_util.h"
#include "statechart/platform/types.h"

namespace state_chart {

class FunctionDispatcher;

// A light weight interpreter for ECMAScript-like expressions.
class LightWeightDatamodel : public Datamodel {
 public:
  // Returns a new datamodel or nullptr if dispatcher is nullptr.
  // Does not take ownership of 'dispatcher'.
  static std::unique_ptr<LightWeightDatamodel> Create(
      FunctionDispatcher* dispatcher);

  // Returns a datamodel initialized from a serialized representation.
  // Does not take ownership of 'dispatcher'.
  // Returns nullptr if 'dispatcher' is null or 'datamodel' parsing failed.
  static std::unique_ptr<LightWeightDatamodel> Create(
      const string& serialized_data, FunctionDispatcher* dispatcher);

  LightWeightDatamodel(const LightWeightDatamodel&) = delete;
  LightWeightDatamodel& operator=(const LightWeightDatamodel&) = delete;
  ~LightWeightDatamodel() override = default;

  bool IsDefined(const string& location) const override;
  // Returns false if attempting to declare a variable location results in a
  // conflicting type of an already declared location. For example, if 'foo'
  // was declared and assigned to '5', declaring 'foo[0]' will return false
  // since 'foo' is not an array.
  bool Declare(const string& location) override;
  // When assigning an expression, a 'location' expression with
  // element access operators [] is allowed to create new locations if the
  // parent object or array exists. If 'expr' is empty, it is treated as a
  // 'null' value.
  bool AssignExpression(const string& location, const string& expr) override;
  bool AssignString(const string& location, const string& expr) override;

  bool EvaluateBooleanExpression(const string& expr,
                                 bool* result) const override;
  bool EvaluateStringExpression(const string& expr,
                                string* result) const override;
  bool EvaluateExpression(const string& expr, string* result) const override;

  string EncodeParameters(
      const std::map<string, string>& parameters) const override;

  // A JSON formatted debug string of the store.
  string DebugString() const override;

  // Clears all data in the 'store_'.
  void Clear() override;

  // Returns a copy of this datamodel. The clone points to the same
  // FunctionDispatcher and Runtime and will be valid only until the original
  // FunctionDispatcher and Runtime remain valid.
  Datamodel* Clone() const override;

  // Returns a string representation used for serializing the contents of the
  // datamodel. The FunctionDispatcher pointed to by 'dispatcher_' is not
  // serialized.
  string SerializeAsString() const override;

  Iterator* EvaluateIterator(const string& location) const;

  // Declares a variable 'location' in the store and assigns 'value'
  // (initializes) to the variable.
  // Returns false if creating the location failed.
  bool DeclareAndAssignJson(const string& location, const Json::Value& value);

  // Evaluate an expression and sets 'result' to the evaluated Json::Value
  // result. Returns false if an error has occurred.
  bool EvaluateJsonExpression(const string& expr, Json::Value* result) const;

  // Returns a const-pointer to the Runtime associated with this datamodel, if
  // present. Returns nullptr otherwise.
  const Runtime* GetRuntime() const override {
    return runtime_;
  }

  // Associates this datamodel with a given Runtime.
  void SetRuntime(const Runtime* runtime) override {
    runtime_ = runtime;
  }

 protected:
  // Returns true if a location is assignable given the current state of the
  // store. A location is assignable if any of the following is true:
  // - IsDefined(location).
  // - 'location' ends in an array element access and the parent is an array.
  // - 'location' ends in an object element access and the parent is an object.
  bool IsAssignable(const string& location) const;

  // Assign a Json value to a location expression.
  // Works the same way as AssignExpression() except that the value is already
  // a Json::Value.
  // Returns false if this location is non-assignable.
  bool AssignJson(const string& location, const Json::Value& value);

  // Initializes datamodel from SerializeAsString() representation.
  // Returns true if parsing is successful.
  bool ParseFromString(const string& data) override;

 private:
  // Does not take ownership of 'dispatcher'.
  // 'dispatcher' must be non-null.
  explicit LightWeightDatamodel(FunctionDispatcher* dispatcher);

  // Storage for locations. This holds the root JSON object.
  Json::Value store_;

  // A pointer to the runtime, this datamodel is associated with.
  const Runtime* runtime_ = nullptr;

  // A dispatcher for C++ function calls.
  FunctionDispatcher* const dispatcher_;
};

// Internal functions, do not use.
namespace internal {

// Converts an expression into a list of string tokens. Tokens are either
// operators or operands. Excess trailing and leading whitespace for each token
// are stripped.
void TokenizeExpression(const string& expr, std::list<string>* tokens);

// Base array iterator class implements functionality but no public CTOR.
// Derived classes implement public CTOR for efficiency.
template <class ArrayType>
class BaseArrayIterator : public Iterator {
 public:
  ~BaseArrayIterator() override = default;

  bool AtEnd() const override { return index_ >= static_cast<int>(array_.size()); }

  bool Next() override {
    if (AtEnd()) return false;
    ++index_;
    return true;
  }

  string GetValue() const override {
    RETURN_VALUE_IF_MSG(
        AtEnd(), "", "Returning empty string; Accessing out of bounds value.");
    string value = Json::FastWriter().write(array_[index_]);
    absl::StripAsciiWhitespace(&value);
    return value;
  }

  string GetIndex() const override { return absl::StrCat(index_); }

 protected:
  BaseArrayIterator() = default;
  explicit BaseArrayIterator(ArrayType array) : array_(array) {}

  // The type of the array may be changed for efficiency.
  ArrayType array_;

 private:
  int index_ = 0;
};

// LightWeightDatamodel Iterator for an array by reference.
class ArrayReferenceIterator : public BaseArrayIterator<const Json::Value&> {
 public:
  explicit ArrayReferenceIterator(const Json::Value& array)
      : BaseArrayIterator(array) {}
};

// LightWeightDatamodel for an array by value.
class ArrayValueIterator : public BaseArrayIterator<Json::Value> {
 public:
  // Internal 'array' will be moved into 'array_'.
  // TODO(qplau): Change to r-value CTOR (move CTOR) when allowed.
  explicit ArrayValueIterator(Json::Value* array) { array->swap(array_); }
};

}  // namespace internal

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_LIGHT_WEIGHT_DATAMODEL_H_
