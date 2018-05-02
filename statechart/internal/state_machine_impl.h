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

#ifndef STATE_CHART_INTERNAL_STATE_MACHINE_IMPL_H_
#define STATE_CHART_INTERNAL_STATE_MACHINE_IMPL_H_

#include <memory>
#include <string>

#include "statechart/state_machine.h"

namespace state_chart {

class Executor;
class Model;
class Runtime;
class StateMachineListener;

class StateMachineImpl : public StateMachine {
 public:
  // Returns a StateMachine generated from given 'executor', 'model' &
  // 'runtime'.
  // All input params must be non-null. The executor and the model must outlive
  // the constructed StateMachineImpl instance.
  // Returns nullptr if StateMachine cannot be created.
  static std::unique_ptr<StateMachine> Create(const Executor* executor,
                                              const Model* model,
                                              std::unique_ptr<Runtime> runtime);
  StateMachineImpl(const StateMachineImpl&) = delete;
  StateMachineImpl& operator=(const StateMachineImpl&) = delete;
  ~StateMachineImpl() override = default;

  void Start() override;

  void SendEvent(const string& event, const string& payload) override;

  void AddListener(StateMachineListener* listener) override;

  const Runtime& GetRuntime() const override;

  const Model& GetModel() const override;

 private:
  StateMachineImpl(const Executor* executor, const Model* model,
                   std::unique_ptr<Runtime> runtime);

  const Executor* const executor_;  // Not owned.
  const Model* const model_;  // Not owned.
  std::unique_ptr<Runtime> runtime_;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_STATE_MACHINE_IMPL_H_
