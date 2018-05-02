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

#ifndef STATE_CHART_INTERNAL_MODEL_STATE_H_
#define STATE_CHART_INTERNAL_MODEL_STATE_H_

#include <string>
#include <vector>

#include "statechart/platform/types.h"
#include "statechart/internal/model/model_element.h"

namespace state_chart {
namespace model {

class ExecutableContent;
class Transition;

// Basic implementation of the State.
class State : public ModelElement {
 public:
  // Params:
  //  id: Unique state id.
  //  is_final: Marks this state as a final state.
  //  is_parallel: Marks this state as a parallel state.
  //  datamodel: The datamodel element.
  //  on_entry: Executable block for onentry.
  //  on_exit: Executable block for onexit.
  State(const string& id, bool is_final, bool is_parallel,
        const ExecutableContent* datamodel, const ExecutableContent* on_entry,
        const ExecutableContent* on_exit);
  State(const State&) = delete;
  State& operator=(const State&) = delete;

  std::vector<const Transition*>* mutable_transitions() {
    return &transitions_;
  }

  // Add the given state as a child state and set this as the parent.
  void AddChild(State* state) {
    child_states_.push_back(state);
    state->SetParent(this);
  }

  // Add all the given child states as children.
  void AddChildren(const std::vector<State*>& child_states);

  string id() const { return id_; }
  bool IsFinal() const { return is_final_; }
  bool IsParallel() const { return is_parallel_; }
  const std::vector<const Transition*>& GetTransitions() const {
    return transitions_;
  }

  const Transition* GetInitialTransition() const { return initial_transition_; }
  // Sets the transition and returns false if IsFinal() or
  // 'initial_transition->GetSourceState() != this'.
  bool SetInitialTransition(const Transition* initial_transition);

  void SetParent(const State* parent) { parent_ = parent; }
  const State* GetParent() const { return parent_; }
  const std::vector<const State*>& GetChildren() const { return child_states_; }
  bool IsAtomic() const { return child_states_.empty(); }
  bool IsCompound() const { return !IsAtomic() && !IsParallel(); }
  const ExecutableContent* GetDatamodelBlock() const { return datamodel_; }
  const ExecutableContent* GetOnEntry() const { return on_entry_; }
  const ExecutableContent* GetOnExit() const { return on_exit_; }

 private:
  const string id_;
  const bool is_final_;
  const bool is_parallel_;
  const State* parent_;
  const Transition* initial_transition_;
  std::vector<const Transition*> transitions_;
  std::vector<const State*> child_states_;
  const ExecutableContent* datamodel_;
  const ExecutableContent* on_entry_;
  const ExecutableContent* on_exit_;
};

}  // namespace model
}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_MODEL_STATE_H_
