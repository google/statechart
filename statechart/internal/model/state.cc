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

#include "statechart/internal/model/state.h"

#include "statechart/internal/model/transition.h"
#include "statechart/logging.h"

namespace state_chart {
namespace model {

State::State(const string& id, bool is_final, bool is_parallel,
             const ExecutableContent* datamodel,
             const ExecutableContent* on_entry,
             const ExecutableContent* on_exit)
    : id_(id),
      is_final_(is_final),
      is_parallel_(is_parallel),
      parent_(nullptr),
      initial_transition_(nullptr),
      datamodel_(datamodel),
      on_entry_(on_entry),
      on_exit_(on_exit) {}

bool State::SetInitialTransition(const Transition* initial_transition) {
  RETURN_FALSE_IF_MSG(
      IsFinal(), "Final (atomic) states may not have an initial transition.");
  RETURN_FALSE_IF(initial_transition->GetSourceState() != this);
  initial_transition_ = initial_transition;
  return true;
}

void State::AddChildren(const std::vector<State*>& child_states) {
  for (auto* child : child_states) {
    AddChild(child);
  }
}

}  // namespace model
}  // namespace state_chart
