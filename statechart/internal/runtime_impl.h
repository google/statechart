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

#ifndef STATE_CHART_INTERNAL_RUNTIME_IMPL_H_
#define STATE_CHART_INTERNAL_RUNTIME_IMPL_H_

#include <list>
#include <memory>
#include <set>
#include <string>
#include <utility>

#include "statechart/platform/types.h"
#include "statechart/internal/event_dispatcher.h"
#include "statechart/internal/runtime.h"
#include "statechart/proto/state_machine_context.pb.h"

namespace state_chart {
class Datamodel;
namespace model {
class State;
}  // namespace model
}  // namespace state_chart

namespace state_chart {

class RuntimeImpl : public Runtime {
 public:
  // Creates a Runtime with given 'datamodel'.
  // Returns nullptr, if datamodel is null.
  static std::unique_ptr<Runtime> Create(std::unique_ptr<Datamodel> datamodel);
  RuntimeImpl(const RuntimeImpl&) = delete;
  RuntimeImpl& operator=(const RuntimeImpl&) = delete;
  ~RuntimeImpl() override = default;

  // Get the list of currently active states.
  std::set<const model::State*> GetActiveStates() const override {
    return active_states_;
  }

  // Returns true iff the state with id 'state_id' is currently active.
  bool IsActiveState(const string& state_id) const override;

  // Add an active state to the active states.
  // Does nothing if state is already an active state.
  void AddActiveState(const model::State* state) override;

  // Erase an active state from the list of active states.
  // Does nothing if the state is not an active state.
  void EraseActiveState(const model::State* state) override;

  // Flag to indicate state machine is running.
  bool IsRunning() const override { return is_running_; }
  void SetRunning(bool is_running) override { is_running_ = is_running; }

  // Returns true if the internal event queue is not empty.
  bool HasInternalEvent() const override;

  // Returns a pair of event id and payload.
  // Returns a pair of empty strings if !HasInternalEvent().
  std::pair<string, string> DequeueInternalEvent() override;

  // Append an internal event to a FIFO queue.
  void EnqueueInternalEvent(const string& event,
                            const string& payload) override;

  // Returns the Datamodel in use for this instance.
  Datamodel* mutable_datamodel() override { return datamodel_.get(); }
  const Datamodel& datamodel() const override { return *datamodel_; }
  EventDispatcher* GetEventDispatcher() override {
    return &listener_event_dispatcher_;
  }

  void Clear() override;

  // Returns a string describing only the active state ids and internal event
  // queue.
  string DebugString() const override;

  // Serializes the internal state of the runtime.
  StateMachineContext::Runtime Serialize() const override;

 private:
  // Creates a RuntimeImpl from given 'datamodel'.
  // 'datamodel' must be non null.
  explicit RuntimeImpl(std::unique_ptr<Datamodel> datamodel);

  std::set<const model::State*> active_states_;

  bool is_running_ = false;

  // FIFO queue.
  std::list<std::pair<string, string>> internal_events_;

  std::unique_ptr<Datamodel> datamodel_;

  EventDispatcher listener_event_dispatcher_;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_RUNTIME_IMPL_H_
