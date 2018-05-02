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

// IWYU pragma: private, include "src/internal/model/model.h"
// IWYU pragma: friend src/internal/model/*

#ifndef STATE_CHART_INTERNAL_MODEL_STR_OR_EXPR_H_
#define STATE_CHART_INTERNAL_MODEL_STR_OR_EXPR_H_

#include <string>

#include "statechart/platform/types.h"

namespace state_chart {
class Datamodel;
namespace model {

// A struct used to indicate to the compiler that a string is of an expression
// type. This allows StrOrExpr constructors to be overloaded.
struct Expr {
  string expr;
  Expr(const string& expr)  // NOLINT(runtime/explicit)
      : expr(expr) {}
};

// A convenience class used to represent variables that can either be a static
// string or an expression that evaluates to a string type in the datamodel.
class StrOrExpr {
 public:
  // Constructor that creates a static string.
  StrOrExpr(const string& str)  // NOLINT(runtime/explicit)
      : is_expr_(false),
        value_(str) {}

  // Constructor that creates a static string from a c-string.
  StrOrExpr(const char* cstr)  // NOLINT(runtime/explicit)
      : StrOrExpr(string(cstr)) {}

  // Constructor that creates an expression.
  StrOrExpr(const Expr& expr)  // NOLINT(runtime/explicit)
      : is_expr_(true),
        value_(expr.expr) {}

  // Constructs a StrOrExpr from 'str' and 'expr' parameters of which only one
  // should be non-empty.
  StrOrExpr(const string& str, const string& expr);

  // Sets 'result' to the string if this is a static string, or a string
  // result of the expression evaluated in the datamodel.
  // Returns false if evaluation failed in the datamodel.
  bool Evaluate(const Datamodel* datamodel, string* result) const;

  bool IsEmpty() const { return value_.empty(); }
  const string& Value() const { return value_; }

 private:
  bool is_expr_;
  string value_;
};

}  // namespace model
}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_MODEL_STR_OR_EXPR_H_
