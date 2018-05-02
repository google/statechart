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

#include "statechart/internal/model/transition.h"

#include <string>

#include "statechart/internal/datamodel.h"
#include "statechart/internal/model/state.h"
#include "statechart/internal/runtime.h"
#include "statechart/internal/utility.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/substitute.h"

namespace state_chart {
namespace model {

Transition::Transition(const State* source,
                       const std::vector<const State*>& targets,
                       const std::vector<string>& events,
                       const string& cond_expr, bool is_internal,
                       const ExecutableContent* executable)
    : source_(source),
      targets_(targets),
      events_(events),
      cond_expr_(cond_expr),
      is_internal_(is_internal),
      executable_(executable) {}

// virtual
bool Transition::EvaluateCondition(Runtime* runtime) const {
  if (cond_expr_.empty()) {
    return true;
  }
  bool result = false;
  if (!runtime->datamodel().EvaluateBooleanExpression(cond_expr_, &result)) {
    runtime->EnqueueExecutionError(
        absl::StrCat("'Transition' condition evaluation failed: ", cond_expr_));
    return false;
  }
  return result;
}

string Transition::DebugString() const {
  std::vector<string> target_ids(targets_.size());
  for (unsigned int i = 0; i < targets_.size(); ++i) {
    target_ids.push_back(targets_[i]->id());
  }
  return absl::Substitute("$0 --> [$1] : events = [$2], cond = <$3>",
                          GetSourceState()->id(),
                          absl::StrJoin(target_ids, ","),
                          absl::StrJoin(GetEvents(), " "), GetCondition());
}

}  // namespace model
}  // namespace state_chart
