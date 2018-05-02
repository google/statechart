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

#include "statechart/internal/model/if.h"

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "statechart/internal/datamodel.h"
#include "statechart/internal/runtime.h"
#include "statechart/internal/utility.h"
#include "statechart/logging.h"

namespace state_chart {
namespace model {

namespace {

struct KeyFormatter {
  template <typename Pair>
  void operator()(std::string* out, Pair pair) const {
    out->append(pair.first);
  }
};

}  // namespace


If::If(const std::vector<std::pair<string, const ExecutableContent*>>&
           condition_executable)
    : condition_executable_(condition_executable) {}

// override
bool If::Execute(Runtime* runtime) const {
  VLOG(1) << absl::StrCat(
      "If conditions: ",
      absl::StrJoin(condition_executable_, ", ", KeyFormatter()));

  bool saw_empty = false;
  bool no_error = true;
  for (const auto& cond_executable : condition_executable_) {
    RETURN_FALSE_IF_MSG(saw_empty,
                        "Empty conditions in <if> executable must come last.");

    const string& cond = cond_executable.first;
    auto* executable = cond_executable.second;

    bool result = cond.empty();  // Empty cond evaluates to true.
    if (!result &&
        !runtime->datamodel().EvaluateBooleanExpression(cond, &result)) {
      runtime->EnqueueExecutionError(
          absl::StrCat("'If' condition failed to evaluate: ", cond));
      no_error = false;
      continue;
    }

    if (result) {
      // Empty executable blocks are nullptr.
      if (executable != nullptr) {
        executable->Execute(runtime);
      }
      return no_error;
    }

    saw_empty |= cond.empty();
  }
  return no_error;
}

}  // namespace model
}  // namespace state_chart
