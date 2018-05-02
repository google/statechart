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

#ifndef STATE_CHART_INTERNAL_STATE_MACHINE_LOGGER_H_
#define STATE_CHART_INTERNAL_STATE_MACHINE_LOGGER_H_

#include <string>

#include "statechart/platform/types.h"
#include "statechart/state_machine_listener.h"

namespace state_chart {
class Runtime;
namespace model {
class State;
}  // namespace model
}  // namespace state_chart

namespace state_chart {

// Logs state machine changes using VLOG(1).
class StateMachineLogger : public StateMachineListener {
 public:
  StateMachineLogger() = default;
  StateMachineLogger(const StateMachineLogger&) = delete;
  StateMachineLogger& operator=(const StateMachineLogger&) = delete;
  ~StateMachineLogger() override = default;

  void OnStateEntered(const Runtime* runtime,
                      const model::State* state) override;

  void OnStateExited(const Runtime* runtime,
                     const model::State* state) override;

  void OnTransitionFollowed(const Runtime* runtime,
                            const model::Transition* transition) override;

  void OnSendEvent(const Runtime* runtime, const string& event,
                   const string& target, const string& type, const string& id,
                   const string& data) override;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_STATE_MACHINE_LOGGER_H_
