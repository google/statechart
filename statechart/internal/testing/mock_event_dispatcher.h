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

#ifndef STATE_CHART_INTERNAL_TESTING_MOCK_EVENT_DISPATCHER_H_
#define STATE_CHART_INTERNAL_TESTING_MOCK_EVENT_DISPATCHER_H_

#include <gmock/gmock.h>
#include <string>

#include "statechart/internal/event_dispatcher.h"
#include "statechart/platform/types.h"

namespace state_chart {

class Runtime;

namespace model {
class State;
class Transition;
}  // namespace model

class MockEventDispatcher : public EventDispatcher {
 public:
  ~MockEventDispatcher() override = default;

  MOCK_METHOD2(NotifyStateEntered, void(const Runtime*, const model::State*));
  MOCK_METHOD2(NotifyStateExited, void(const Runtime*, const model::State*));
  MOCK_METHOD2(NotifyTransitionFollowed,
               void(const Runtime*, const model::Transition*));

  MOCK_METHOD6(NotifySendEvent,
               void(const Runtime*, const string&, const string&, const string&,
                    const string&, const string&));
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_TESTING_MOCK_EVENT_DISPATCHER_H_
