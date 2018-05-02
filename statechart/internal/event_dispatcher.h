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

#ifndef STATE_CHART_INTERNAL_EVENT_DISPATCHER_H_
#define STATE_CHART_INTERNAL_EVENT_DISPATCHER_H_

#include <string>
#include <vector>

#include "statechart/platform/types.h"

namespace state_chart {

class Runtime;
class StateMachineListener;

namespace model {
class State;
class Transition;
}  // namespace model

// This class is responsible for collecting listeners for a specific instance of
// a state machine, and dispatching events when they occur. The state machine
// system invokes the various Notify*() methods, which are in turn passed to the
// equivalent On*() methods in the listeners.
class EventDispatcher {
 public:
  virtual ~EventDispatcher();

  // Does not take ownership of 'listener'.
  void AddListener(StateMachineListener* listener);

  virtual void NotifyStateEntered(const Runtime* runtime,
                                  const model::State* state);

  virtual void NotifyStateExited(const Runtime* runtime,
                                 const model::State* state);

  virtual void NotifyTransitionFollowed(const Runtime* runtime,
                                        const model::Transition* transition);

  virtual void NotifySendEvent(const Runtime* runtime, const string& event,
                               const string& target, const string& type,
                               const string& id, const string& data);

 private:
  std::vector<StateMachineListener*> listeners_;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_EVENT_DISPATCHER_H_
