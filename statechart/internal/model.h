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

#ifndef STATE_CHART_INTERNAL_MODEL_H_
#define STATE_CHART_INTERNAL_MODEL_H_

#include <set>
#include <string>
#include <vector>

#include "statechart/platform/types.h"
#include "statechart/platform/protobuf.h"
#include "statechart/proto/state_chart.pb.h"
#include "statechart/proto/state_machine_context.pb.h"

namespace state_chart {
class Runtime;
namespace model {
class ExecutableContent;
class State;
class Transition;
}  // namespace model
}  // namespace state_chart

namespace state_chart {

// The model encapsulates the state machine specification in memory. It answers,
// queries relating to the specification. It also manages the memory of all
// model related objects.
class Model {
 public:
  Model() = default;
  Model(const Model&) = delete;
  Model& operator=(const Model&) = delete;
  virtual ~Model() = default;

  // Returns true if a given 'event_name' matches any event specifications in
  // the given list, 'events'. Matching is based on the state chart rules, such
  // that descendant events matches all ancestor events, e.g., the 'event_name'
  // "A.B" matches the specification "A". The "*" matches any 'event_name'.
  static bool EventMatches(const string& event_name,
                           const std::vector<string>& events);

  // Obtain the name of the model.
  virtual string GetName() const = 0;

  // Based on the active states and datamodel in 'runtime', returns any
  // transitions that can be followed without having been triggered by an event.
  // Error events may be enqueued in 'runtime' if there are errors evaluating
  // transition conditions.
  virtual std::vector<const model::Transition*> GetEventlessTransitions(
      Runtime* runtime) const = 0;

  // Based on the active states and datamodel in 'runtime', returns any
  // transitions that can be followed given the triggering 'event'.
  // Error events may be enqueued in 'runtime' if there are errors evaluating
  // transition conditions.
  virtual std::vector<const model::Transition*> GetTransitionsForEvent(
      Runtime* runtime, const string& event) const = 0;

  // Returns the initial transition executed when a new runtime is started.
  virtual const model::Transition* GetInitialTransition() const = 0;

  // Returns the datamodel section of the state chart, nullptr if it does not
  // exists.
  virtual const model::ExecutableContent* GetDatamodelBlock() const = 0;

  // Returns the datamodel binding.
  virtual config::StateChart::Binding GetDatamodelBinding() const = 0;

  // Returns the top-level states.
  virtual std::vector<const model::State*> GetTopLevelStates() const = 0;

  // Returns pointers to state(s) for a given tree of active states.
  virtual std::vector<const model::State*> GetActiveStates(
      const proto2::RepeatedPtrField<
          StateMachineContext::Runtime::ActiveStateElement>& active_states)
      const = 0;

  // Computes the entry set for a list of transitions and stores results in
  // 'states_to_enter' for states, that will be entered and
  // 'states_for_default_entry' that contains the compound states to be entered.
  // The states in 'states_to_enter' will be sorted in "entry order", that is:
  // Ancestors precede descendants, with document order being used to break ties
  // (Note: since ancestors precede descendants, this is equivalent to document
  // order.)
  // Returns false if error occurred.
  virtual bool ComputeEntrySet(
      const Runtime* runtime,
      const std::vector<const model::Transition*>& transitions,
      std::vector<const model::State*>* states_to_enter,
      std::set<const model::State*>* states_for_default_entry) const = 0;

  // Computes the exit set of a list of transitions and returns results in
  // "exit order".
  // Exit order is defined as: Descendants precede ancestors, with reverse
  // document order being used to break ties (Note: since descendants follow
  // ancestors, this is equivalent to reverse document order.).
  virtual std::vector<const model::State*> ComputeExitSet(
      const Runtime* runtime,
      const std::vector<const model::Transition*>& transitions) const = 0;

  // Sort a vector of states into document order.
  // Set reverse to true to sort in reverse document order.
  virtual void SortStatesByDocumentOrder(
      bool reverse, std::vector<const model::State*>* states) const = 0;

  // Return true if 'state' is a compound State and one of its children is an
  // active Final state (i.e. is a member of the current configuration), or if
  // 'state' is a Parallel state and IsInFinalState is true of all its children.
  virtual bool IsInFinalState(const Runtime* runtime,
                              const model::State* state) const = 0;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_MODEL_H_
