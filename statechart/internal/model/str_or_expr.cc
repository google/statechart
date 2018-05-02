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

#include "statechart/internal/model/str_or_expr.h"

#include "statechart/internal/datamodel.h"

namespace state_chart {
namespace model {

StrOrExpr::StrOrExpr(const string& str, const string& expr) {
  is_expr_ = !expr.empty();
  value_ = is_expr_ ? expr : str;
}

bool StrOrExpr::Evaluate(const Datamodel* datamodel, string* result) const {
  if (is_expr_) {
    return datamodel->EvaluateStringExpression(value_, result);
  }
  *result = value_;
  return true;
}

}  // namespace model
}  // namespace state_chart
