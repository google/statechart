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

#include "statechart/internal/runtime_impl.h"

#include <vector>

#include "absl/algorithm/container.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_join.h"
#include "absl/strings/substitute.h"
#include "statechart/internal/datamodel.h"
#include "statechart/internal/model/model.h"
#include "statechart/logging.h"
#include "statechart/platform/map_util.h"
#include "statechart/platform/protobuf.h"
#include "statechart/platform/types.h"

namespace state_chart {

namespace {

// Lookup or insert (if not present) an active state with given 'id' to the list
// of 'active_states' and return the pointer to that active state.
StateMachineContext::Runtime::ActiveStateElement* LookupOrInsert(
    const string& id,
    proto2::RepeatedPtrField<StateMachineContext::Runtime::ActiveStateElement>*
        active_states) {
  for (auto& active_state : *active_states) {
    if (active_state.id() == id) {
      return &active_state;
    }
  }
  auto* new_active_state = active_states->Add();
  new_active_state->set_id(id);
  return new_active_state;
}

// Adds the states in provided path to 'active_states'.
void AddStatePathToActiveState(
    const std::vector<string>& state_to_root_path,
    proto2::RepeatedPtrField<StateMachineContext::Runtime::ActiveStateElement>*
        active_states) {
  auto* states = active_states;
  // We iterate through the path in reverse order to get a path from root to the
  // state.
  for (int i = state_to_root_path.size() - 1; i >= 0; --i) {
    const string& state_id = state_to_root_path[i];
    auto* active_state = LookupOrInsert(state_id, states);
    if (i > 0) {  // Create 'active_child' only if needed.
      states = active_state->mutable_active_child();
    }
  }
}

// Add information about the 'state' and its active children to the list of
// 'active_states'.
void PopulateActiveStateElement(
    const model::State* state,
    proto2::RepeatedPtrField<StateMachineContext::Runtime::ActiveStateElement>*
        active_states) {
  std::vector<string> state_to_root_path;  // A path from current node to root.
  for (const model::State* node = state; node != nullptr;
       node = node->GetParent()) {
    state_to_root_path.push_back(node->id());
  }
  AddStatePathToActiveState(state_to_root_path, active_states);
}

}  // namespace

// static
std::unique_ptr<Runtime> RuntimeImpl::Create(
    std::unique_ptr<Datamodel> datamodel) {
  RETURN_NULL_IF(datamodel == nullptr);
  return absl::WrapUnique(new RuntimeImpl(std::move(datamodel)));
}

RuntimeImpl::RuntimeImpl(std::unique_ptr<Datamodel> datamodel)
    : datamodel_(std::move(datamodel)) {
  datamodel_->SetRuntime(this);
}

// override
bool RuntimeImpl::IsActiveState(const string& state_id) const {
  return ::absl::c_any_of(active_states_,
                          [&state_id](const model::State* state) {
                            return state->id() == state_id;
                          });
}

// override
void RuntimeImpl::AddActiveState(const model::State* state) {
  gtl::InsertIfNotPresent(&active_states_, state);
}

// override
void RuntimeImpl::EraseActiveState(const model::State* state) {
  active_states_.erase(state);
}

// override
bool RuntimeImpl::HasInternalEvent() const { return !internal_events_.empty(); }

// override
std::pair<string, string> RuntimeImpl::DequeueInternalEvent() {
  RETURN_VALUE_IF_MSG(
      !HasInternalEvent(), std::make_pair("", ""),
      "Returning empty string pair; There are no Internal events to dequeue.");
  const std::pair<string, string> event = std::move(internal_events_.front());
  internal_events_.pop_front();
  return event;
}

// override
void RuntimeImpl::EnqueueInternalEvent(const string& event,
                                       const string& payload) {
  internal_events_.push_back(std::make_pair(event, payload));
}

// override
void RuntimeImpl::Clear() {
  datamodel_->Clear();
  internal_events_.clear();
  active_states_.clear();
}

// override
string RuntimeImpl::DebugString() const {
  std::vector<string> state_ids;
  for (const auto& state : active_states_) {
    state_ids.push_back(state->id());
  }
  std::vector<string> events;
  for (const auto& event : internal_events_) {
    events.push_back(absl::Substitute("($0 $1)", event.first, event.second));
  }
  return absl::Substitute(
      "RuntimeImpl\n"
      "  Active States  : $0\n"
      "  Internal Events: $1",
      absl::StrJoin(state_ids, ", "), absl::StrJoin(events, ", "));
}

// override
StateMachineContext::Runtime RuntimeImpl::Serialize() const {
  StateMachineContext::Runtime serialized_runtime;
  if (!HasInternalEvent()) {
    serialized_runtime.set_running(IsRunning());
    for (const auto* active_state : GetActiveStates()) {
      PopulateActiveStateElement(active_state,
                                 serialized_runtime.mutable_active_state());
    }
  } else {
    LOG(DFATAL) << "Trying to serialize a Runtime that has not been allowed to "
                   "run to quiescence is not allowed. Returning empty.";
  }
  return serialized_runtime;
}

}  // namespace state_chart
