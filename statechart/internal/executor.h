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

// The Executor implements most of the SCXML interpretation pseudo-code.
// See: http://www.w3.org/TR/scxml/#AlgorithmforSCXMLInterpretation
#ifndef STATE_CHART_INTERNAL_EXECUTOR_H_
#define STATE_CHART_INTERNAL_EXECUTOR_H_

#include <string>
#include <vector>

#include "statechart/platform/types.h"

namespace state_chart {
class Model;
class Runtime;
namespace model {
class Transition;
}  // namespace model
}  // namespace state_chart

namespace state_chart {

// This class executes the StateChart with the same semantics as the algorithm
// for SCXML interpretation. An Executor instance may be shared for various
// instances of Model and Runtime. In general, StateMachine instances share the
// same Executor instance, may (or may not) share the same Model, and will own
// an instance of Runtime. The methods on Executor reflects this by allowing
// different Model and Runtime instances to be passed in as parameters.
// These input parameters should be non-null. If anyone of them is nullptr no
// action is performed.
class Executor {
 public:
  Executor() = default;
  Executor(const Executor&) = delete;
  Executor& operator=(const Executor&) = delete;
  virtual ~Executor() = default;

  // Start the execution of the machine by entering the initial state
  // and executing until the state machine reaches a stable state. A stable
  // state is one where there are no transitions and no internal events that
  // can be processed.
  virtual void Start(const Model* model, Runtime* runtime) const;

  // Notify the Executor of an external event. Executes transitions based on
  // this event until the state machine reaches a stable state.
  // This corresponds to a 'macro step' in the SCXML algorithm specification.
  // Parameter payload may be an empty string (i.e. no payload).
  virtual void SendEvent(const Model* model, Runtime* runtime,
                         const string& event, const string& payload) const;

 protected:
  // Handles all internal events until the state machine reaches a stable state.
  // This corresponds to mainEventLoop() procedure above from lines 6 to 35.
  // Input params 'model' & 'runtime' must be non-null.
  virtual void ExecuteUntilStable(const Model* model, Runtime* runtime) const;

  // Handles a single external event. This corresponds to the mainEventLoop()
  // procedure above from lines 40 to 53.
  // Parameter payload may be an empty string (i.e. no payload).
  // Input params 'model' & 'runtime' must be non-null.
  virtual void ProcessExternalEvent(const Model* model, Runtime* runtime,
                                    const string& event,
                                    const string& payload) const;

  // Execute a microstep. This takes a list of transitions, exits all their
  // source states, runs their executable blocks and finally enters all the
  // target states.
  // Input params 'model' & 'runtime' must be non-null.
  virtual void MicroStep(
      const Model* model, Runtime* runtime,
      const std::vector<const model::Transition*>& transitions) const;

  // Enters the target states of the transitions, calling their <onentery>
  // executable blocks.
  // Input params 'model' & 'runtime' must be non-null.
  virtual void EnterStates(
      const Model* model, Runtime* runtime,
      const std::vector<const model::Transition*>& transitions) const;

  // Exits the source states of the transitions, calling <onexit> executable
  // blocks.
  // Input params 'model' & 'runtime' must be non-null.
  virtual void ExitStates(
      const Model* model, Runtime* runtime,
      const std::vector<const model::Transition*>& transitions) const;

  // Assigns event data to the '_event' variable in the runtime's datamodel.
  // Parameters:
  //   event   The event name.
  //   payload The event data.
  //
  // Input params 'model' & 'runtime' must be non-null.
  virtual void AssignEventData(Runtime* runtime, const string& event,
                               const string& payload) const;

  // Shutdown the state machine by exiting all states in the correct order.
  // Input params 'model' & 'runtime' must be non-null.
  virtual void Shutdown(const Model* model, Runtime* runtime) const;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_EXECUTOR_H_
