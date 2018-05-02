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

#include "statechart/internal/state_machine_logger.h"

#include "absl/strings/str_join.h"
#include "absl/strings/substitute.h"
#include "statechart/internal/datamodel.h"
#include "statechart/internal/model/model.h"
#include "statechart/internal/runtime.h"
#include "statechart/platform/types.h"

namespace state_chart {

namespace {

// Returns a string containing the state chart name and session id.
string GetStateChartId(const Runtime* runtime) {
  string name, id;
  const auto& datamodel = runtime->datamodel();
  datamodel.EvaluateStringExpression("_name", &name);
  datamodel.EvaluateStringExpression("_sessionid", &id);
  return absl::Substitute("[$0][$1]", id, name);
}

}  // namespace

void StateMachineLogger::OnStateEntered(const Runtime* runtime,
                                        const model::State* state) {
  VLOG(1) << absl::Substitute("$0 State entered: $1", GetStateChartId(runtime),
                              state->id());
}

void StateMachineLogger::OnStateExited(const Runtime* runtime,
                                       const model::State* state) {
  VLOG(1) << absl::Substitute("$0 State exited: $1", GetStateChartId(runtime),
                              state->id());
}

void StateMachineLogger::OnTransitionFollowed(
    const Runtime* runtime, const model::Transition* transition) {
  std::vector<string> targets;
  for (const auto* state : transition->GetTargetStates()) {
    targets.emplace_back(state->id());
  }
  VLOG(1) << absl::Substitute(
      "$0 Transition followed: cond=\"$1\", source=[$2], targets=[$3]",
      GetStateChartId(runtime), transition->GetCondition(),
      transition->GetSourceState() == nullptr
          ? "null"
          : transition->GetSourceState()->id(),
      absl::StrJoin(targets, ", "));
}

void StateMachineLogger::OnSendEvent(const Runtime* runtime,
                                     const string& event, const string& target,
                                     const string& type, const string& id,
                                     const string& data) {
  VLOG(1) << absl::Substitute(
      "$0 Send event: event=$1, target=$2, type=$3, id=$4, data=$5",
      GetStateChartId(runtime), event, target, type, id, data);
}

}  // namespace state_chart
