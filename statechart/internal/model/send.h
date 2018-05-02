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

#ifndef STATE_CHART_INTERNAL_MODEL_SEND_H_
#define STATE_CHART_INTERNAL_MODEL_SEND_H_

#include <map>
#include <string>

#include "statechart/platform/types.h"
#include "statechart/internal/model/executable_content.h"
#include "statechart/internal/model/str_or_expr.h"

namespace state_chart {
class Runtime;
}

namespace state_chart {
namespace model {

// This class implements the Send action. It contains a key-value map of data
// parameters to be passed to the external service. All expressions are
// evaluated against the current datamodel (in runtime) when Execute() is
// called. Then listeners of Send are notified immediately.
// Note that Execute() will not notify listeners if the evaluation of any
// attribute fails. However, when the evaluation of a parameter fails,
// notification proceeds with the failing parameter ignored. This behaviour
// is defined in the state chart specification for <param>.
class Send : public ExecutableContent {
 public:
  // Create send with its attributes that can either be a static string or an
  // expression to be evaluated at runtime.
  Send(const StrOrExpr& event, const StrOrExpr& target, const StrOrExpr& id,
       const StrOrExpr& type);
  Send(const Send&) = delete;
  Send& operator=(const Send&) = delete;

  ~Send() override = default;

  bool Execute(Runtime* runtime) const override;

  // Add a parameter by giving it a key (name) and a expression to evaluate
  // in the datamodel.
  // Does nothing if key was already added. Returns false if 'expr' is empty.
  // Note that this behaviour differs from the state chart specification as the
  // specification allows duplicate parameters.
  // Returns false if an error occurred.
  bool AddParamByExpression(const string& key, const string& expr);

  // Add a parameter based on a datamodel location name. The key of the
  // parameter is the name of the datamodel location and the expression is the
  // datamodel location (that evaluates to its value when this is executed).
  // Does nothing if key was already added.
  void AddParamById(const string& location) {
    AddParamByExpression(location, location);
  }

 private:
  // Send attribute expressions.
  StrOrExpr event_;
  StrOrExpr target_;
  StrOrExpr id_;
  StrOrExpr type_;

  // A map of parameter name to value expressions to be evaluated before being
  // passed to an external service when send is executed.
  std::map<string, string> parameters_;
};

}  // namespace model
}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_MODEL_SEND_H_
