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

#ifndef STATE_CHART_STATE_MACHINE_LISTENER_H_
#define STATE_CHART_STATE_MACHINE_LISTENER_H_

#include <string>

#include "statechart/platform/types.h"

namespace state_chart {

class Runtime;

namespace model {
class State;
class Transition;
}  // namespace model

// A base class for clients that wish to listen on events and changes
// of state within a running state machine. Treat this class as an interface.
// Default NO-OP implementations are given out of convenience to the user.
//
// Sub-class this class and override methods of interest. Pass instances to
// StateMachine::AddListener() to listen on a specific running state machine.
class StateMachineListener {
 public:
  virtual ~StateMachineListener() {}

  // Invoked when a state is entered (becomes active), after the state's
  // "onentry" executable content has been executed.
  virtual void OnStateEntered(const Runtime* runtime,
                              const model::State* state) {}

  // Invoked when a state is exited (becomes inactive), after the state's
  // "onexit" executable content has been executed.
  virtual void OnStateExited(const Runtime* runtime,
                             const model::State* state) {}

  // Invoked when a transition is followed. Does not fire for "initial
  // transitions" (a transition that has no source state and whose target state
  // is the initial/start state of a StateMachine or of a compound or parallel
  // state within that StateMachine).
  // This is called after the source state has been exited (if applicable)
  // and the transition's executable content has been called, but before
  // the target state is entered (if applicable).
  virtual void OnTransitionFollowed(const Runtime* runtime,
                                    const model::Transition* transition) {}

  // Invoked every time a state machine executes a <send> action. See:
  // http://www.w3.org/TR/scxml/#send
  //
  // The encoding of 'data' is specific to the specification of the <send>
  // instance. If the <send> element specifies a 'namelist' attribute, or
  // contains within it child <param> elements, 'data' will be encoded as a JSON
  // dictionary corresponding to the values in the runtime's datamodel for each
  // location listed in 'namelist' or in the <params>. If the <send> element
  // specifies a <content> child, the content is included as-is with no
  // modification.
  virtual void OnSendEvent(const Runtime* runtime,
                           const string& event,
                           const string& target,
                           const string& type,
                           const string& id,
                           const string& data) {}
};

}  // namespace state_chart

#endif  // STATE_CHART_STATE_MACHINE_LISTENER_H_
