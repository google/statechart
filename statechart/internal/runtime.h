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

#ifndef STATE_CHART_INTERNAL_RUNTIME_H_
#define STATE_CHART_INTERNAL_RUNTIME_H_

#include <set>
#include <string>
#include <utility>

#include "statechart/platform/types.h"
#include "statechart/proto/state_machine_context.pb.h"

namespace state_chart {
class Datamodel;
class EventDispatcher;
namespace model {
class State;
}  // namespace model
}  // namespace state_chart

namespace state_chart {

// Runtime holds all runtime information (state) of an executing state machine.
class Runtime {
 public:
  virtual ~Runtime() {}

  // Get the list of currently active states.
  virtual std::set<const model::State*> GetActiveStates() const = 0;

  // Returns true iff the state with id 'state_id' is currently active.
  virtual bool IsActiveState(const string& state_id) const = 0;

  // Add an active state to the active states.
  // Does nothing if state is already an active state.
  virtual void AddActiveState(const model::State* state) = 0;

  // Erase an active state from the list of active states.
  // Does nothing if the state is not an active state.
  virtual void EraseActiveState(const model::State* state) = 0;

  // Flag to indicate state machine is running.
  virtual bool IsRunning() const = 0;
  virtual void SetRunning(bool is_running) = 0;

  // Returns true if the internal event queue is not empty.
  virtual bool HasInternalEvent() const = 0;

  // Returns a pair of event id and payload.
  // Returns a pair of empty strings if !HasInternalEvent().
  // TODO(qplau): Event should be a struct or class to hold other information
  //              such as 'type'.
  virtual std::pair<string, string> DequeueInternalEvent() = 0;

  // Append an internal event to a FIFO queue.
  virtual void EnqueueInternalEvent(const string& event,
                                    const string& payload) = 0;

  // Returns the Datamodel in use for this instance.
  virtual Datamodel* mutable_datamodel() = 0;
  virtual const Datamodel& datamodel() const = 0;

  // Returns the ListenerEventDispatcher in use for this instance.
  virtual EventDispatcher* GetEventDispatcher() = 0;

  // Clears all data in the runtime, including the data in the datamodel.
  virtual void Clear() = 0;

  // Debug string of the data contained in the runtime.
  virtual string DebugString() const = 0;

  // Enqueue an "error.execution" event with a Json payload given the error
  // string. 'error_msg' will be prepended with "[datamodel] ".
  void EnqueueExecutionError(const string& error_msg);

  // Serializes the internal state of the runtime.
  virtual StateMachineContext::Runtime Serialize() const = 0;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_RUNTIME_H_
