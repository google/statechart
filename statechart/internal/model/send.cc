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

#include "statechart/internal/model/send.h"

#include <glog/logging.h>

#include "absl/strings/str_cat.h"
#include "statechart/internal/datamodel.h"
#include "statechart/internal/event_dispatcher.h"
#include "statechart/internal/runtime.h"
#include "statechart/internal/utility.h"
#include "statechart/logging.h"
#include "statechart/platform/map_util.h"

namespace state_chart {
namespace model {

Send::Send(const StrOrExpr& event, const StrOrExpr& target, const StrOrExpr& id,
           const StrOrExpr& type)
    : event_(event), target_(target), id_(id), type_(type) {}

// override
bool Send::Execute(Runtime* runtime) const {
  VLOG(1) << "Send: event = " << event_.Value()
          << ", target = " << target_.Value();
  Datamodel* datamodel = runtime->mutable_datamodel();

  // Evaluate string expressions for send attributes.
  static const char* const kAttributeName[] = {"event", "target", "type", "id"};
  const StrOrExpr* string_attr[] = {&event_, &target_, &type_, &id_};

  static_assert(ABSL_ARRAYSIZE(kAttributeName) == ABSL_ARRAYSIZE(string_attr),
                "Array sizes must be equal.");

  std::vector<string> string_attr_value(ABSL_ARRAYSIZE(string_attr), "");

  for (unsigned int i = 0; i < ABSL_ARRAYSIZE(string_attr); ++i) {
    if (!string_attr[i]->IsEmpty() &&
        !string_attr[i]->Evaluate(datamodel, &string_attr_value[i])) {
      runtime->EnqueueExecutionError(absl::StrCat(
          "'Send' attribute '", kAttributeName[i],
          "' failed to evaluate value: ", string_attr[i]->Value()));
      return false;
    }
  }

  // Evaluate expressions for send parameters.
  std::map<string, string> evaluated_data;
  string result;
  bool no_error = true;
  for (const auto& data : parameters_) {
    result.clear();
    // If there is an error, the specifications say to ignore the parameter.
    // Hence we continue to process the rest.
    if (!datamodel->EvaluateExpression(data.second, &result)) {
      runtime->EnqueueExecutionError(
          absl::StrCat("'Send' parameter '", data.first,
                       "' failed to evaluate value: ", data.second));
      no_error = false;
    } else {
      evaluated_data[data.first] = result;
    }
  }

  runtime->GetEventDispatcher()->NotifySendEvent(
      runtime, string_attr_value[0], string_attr_value[1], string_attr_value[2],
      string_attr_value[3], datamodel->EncodeParameters(evaluated_data));
  return no_error;
}

bool Send::AddParamByExpression(const string& key, const string& expr) {
  RETURN_FALSE_IF(expr.empty());
  gtl::InsertIfNotPresent(&parameters_, key, expr);
  return true;
}

}  // namespace model
}  // namespace state_chart
