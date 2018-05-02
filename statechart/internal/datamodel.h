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

#ifndef STATE_CHART_INTERNAL_DATAMODEL_H_
#define STATE_CHART_INTERNAL_DATAMODEL_H_

#include <map>
#include <string>

#include "statechart/platform/types.h"

namespace state_chart {

class Runtime;

// Defines the interface for an iterable collection in the datamodel.
class Iterator {
 public:
  virtual ~Iterator() = default;

  // True if we are at the end of the collection.
  virtual bool AtEnd() const = 0;

  // Advances the iterator. Returns true if successful, false if IsEnd().
  virtual bool Next() = 0;

  // Retrieves the current element's value as a datamodel value expression.
  // Returns empty string if IsEnd().
  virtual string GetValue() const = 0;

  // Retrieves the current element's index as a datamodel value expression.
  virtual string GetIndex() const = 0;

 protected:
  Iterator() = default;
};

// Defines the interface for interacting with a specific Datamodel
// context. This stores a single grouping of variables and values shared by
// all states in a given StateMachine.
class Datamodel {
 public:
  virtual ~Datamodel();

  // Returns true if a 'location' expression evaluates to a defined (existing)
  // location in the datamodel.
  //
  // If using the ECMAScript datamodel (the default), see
  // http://www.w3.org/TR/scxml/#ecma_location_expressions for valid location
  // expressions.
  virtual bool IsDefined(const string& location) const = 0;

  // Used to create variables in the datamodel. The variable is initialized to
  // an implementation specific value. After a variable is successfully
  // declared, IsDefined(location) should always return true.
  // Returns false if the declared location is incompatible with existing
  // declarations.
  virtual bool Declare(const string& location) = 0;

  // Used to assign the result of 'expr' to the datamodel location specified
  // by 'location'. If the variable at 'location' already exists, the value
  // is replaced. Otherwise, deciding if the variable may be created or not is
  // implementation dependent.
  // Returns true if successful and false if an evaluation error occurs.
  //
  // See http://www.w3.org/TR/scxml/#ecma_value_expressions for valid
  // ECMAScript value expressions.
  virtual bool AssignExpression(const string& location, const string& expr) = 0;

  // Assign a 'str' as a string literal to 'location'.
  virtual bool AssignString(const string& location, const string& str) = 0;

  // Sets the value of 'result' to the result of evaluating a simple T/F
  // condition. The condition in 'expr' MUST not have any side-effects. Return
  // false if an evaluation error occurs, and true otherwise.
  virtual bool EvaluateBooleanExpression(const string& expr,
                                         bool* result) const = 0;

  // Sets the value of 'result' to the result of evaluating an expression that
  // returns a string. The expression 'expr' MUST not have any side-effects.
  // Returns false if an evaluation error occurs, and true otherwise.
  virtual bool EvaluateStringExpression(const string& expr,
                                        string* result) const = 0;

  // Evaluates and sets the value of 'result' to the result of evaluating an
  // expression. The expression 'expr' MUST not have any side-effects.
  // The result is also a valid expression in the datamodel, i.e., it can be
  // used in Assign().
  virtual bool EvaluateExpression(const string& expr, string* result) const = 0;

  // Encodes a map of parameter name to value expression into an implementation
  // dependent value expression of the map. For example, 'parameters' may be
  // encoded into a JSON object that is a valid ECMAScript value expression.
  virtual string EncodeParameters(
      const std::map<string, string>& parameters) const = 0;

  // A string representation of the entire datamodel used for debugging.
  virtual string DebugString() const = 0;

  // Resets the datamodel, clearing out all data.
  virtual void Clear() = 0;

  // Returns a copy of this datamodel.
  virtual Datamodel* Clone() const = 0;

  // Returns a string representation used for serializing the contents of the
  // datamodel.
  virtual string SerializeAsString() const = 0;

  // Returns an Iterator to a collection specified by the location expression or
  // nullptr if the location is an invalid collection location. Iterator may
  // become invalid if the underlying collection is modified.
  // Caller takes ownership of result.
  virtual Iterator* EvaluateIterator(const string& location) const = 0;

  // Returns a const-pointer to the Runtime associated with this datamodel.
  // If a datamodel is being used outside the context of state_charts, then this
  // method may return nullptr.
  virtual const Runtime* GetRuntime() const = 0;

  // Associates this datamodel with a given Runtime.
  virtual void SetRuntime(const Runtime* runtime) = 0;

 protected:
  // Initializes datamodel from SerializeAsString() representation.
  // Returns true if parsing is successful.
  // This method is protected to enforce that the restoration of datamodel from
  // its serialized representation should only happen at the construction time.
  virtual bool ParseFromString(const string& data) = 0;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_DATAMODEL_H_
