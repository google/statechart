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

#include "statechart/internal/event_dispatcher.h"

#include "statechart/internal/runtime.h"
#include "statechart/state_machine_listener.h"

#define FOREACH_LISTENER(what) for (const auto& listener : listeners_) { \
  listener->what; \
}

namespace state_chart {

// virtual
EventDispatcher::~EventDispatcher() {}

void EventDispatcher::AddListener(StateMachineListener* listener) {
  listeners_.push_back(listener);
}

// virtual
void EventDispatcher::NotifyStateEntered(const Runtime* runtime,
                                         const model::State* state) {
  FOREACH_LISTENER(OnStateEntered(runtime, state));
}

// virtual
void EventDispatcher::NotifyStateExited(const Runtime* runtime,
                                        const model::State* state) {
  FOREACH_LISTENER(OnStateExited(runtime, state));
}

// virtual
void EventDispatcher::NotifyTransitionFollowed(
    const Runtime* runtime, const model::Transition* transition) {
  FOREACH_LISTENER(OnTransitionFollowed(runtime, transition));
}

// virtual
void EventDispatcher::NotifySendEvent(const Runtime* runtime,
                                      const string& event, const string& target,
                                      const string& type, const string& id,
                                      const string& data) {
  FOREACH_LISTENER(OnSendEvent(runtime, event, target, type, id, data));
}

}  // namespace state_chart
