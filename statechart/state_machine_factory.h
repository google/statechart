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

#ifndef STATE_CHART_STATE_MACHINE_FACTORY_H_
#define STATE_CHART_STATE_MACHINE_FACTORY_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/memory/memory.h"
#include "statechart/internal/state_machine_logger.h"
#include "statechart/proto/state_machine_context.pb.h"
#include "statechart/state_machine_listener.h"

namespace state_chart {
class Executor;
class FunctionDispatcher;
class Model;
class StateMachine;
namespace config {
class StateChart;
}  // namespace config
}  // namespace state_chart

namespace state_chart {

// Manages models and instantiates StateMachines based on these models.
class StateMachineFactory {
 public:
  // Create a factory with models from a list of StateChart protos.
  // Template param 'Range' must be a range type like 'vector' or 'value_view'
  // of a map.
  // It also adds StateMachineListener 'listener' to all StateCharts created.
  // Returns nullptr if any of the StateCharts fails to be added to the factory.
  template <typename Range>
  static std::unique_ptr<StateMachineFactory> CreateFromProtos(
      const Range& state_charts,
      std::unique_ptr<StateMachineListener> listener);

  // Same as above, with a StateMachineLogger as a default listener.
  template <typename Range>
  static std::unique_ptr<StateMachineFactory> CreateFromProtos(
      const Range& state_charts) {
    return CreateFromProtos(state_charts,
                            std::unique_ptr<StateMachineListener>(
                                ::absl::make_unique<StateMachineLogger>()));
  }

  StateMachineFactory(const StateMachineFactory&) = delete;
  StateMachineFactory& operator=(const StateMachineFactory&) = delete;
  ~StateMachineFactory();

  // Create a StateMachine instance given the model name and
  // 'function_dispatcher'.
  // Returns nullptr if !HasModel(model_name).
  // Note that the factory must outlive all StateMachines created from it.
  // Also function_dispatcher should outlive the StateMachine which uses it.
  std::unique_ptr<StateMachine> CreateStateMachine(
      const string& model_name,
      state_chart::FunctionDispatcher* function_dispatcher) const;

  // Same as above but with an additional argument 'state_machine_context'
  // which stores serialized state of the state machine.
  std::unique_ptr<StateMachine> CreateStateMachine(
      const string& model_name,
      const StateMachineContext& state_machine_context,
      state_chart::FunctionDispatcher* function_dispatcher) const;

  // Check that a given model exists, i.e., a state machine can be created from
  // this model name.
  bool HasModel(const string& model_name) const;

 protected:
  StateMachineFactory();
  explicit StateMachineFactory(std::unique_ptr<StateMachineListener> listener);

  // Adds a model from a StateChart proto. If a model with the same model name
  // exists, it will be replaced.
  // Returns false if there is a model creation error or if the state chart has
  // no name field.
  // Returns true if it successfully adds the StateChart Model to factory.
  bool AddModelFromProto(const config::StateChart& state_chart);

 private:
  std::unique_ptr<const Executor> executor_;
  std::unique_ptr<StateMachineListener> listener_;
  std::map<string, std::unique_ptr<const Model>> models_;
};

// static
template <typename Range>
std::unique_ptr<StateMachineFactory> StateMachineFactory::CreateFromProtos(
    const Range& state_charts, std::unique_ptr<StateMachineListener> listener) {
  auto factory =
      ::absl::WrapUnique(new StateMachineFactory(std::move(listener)));
  for (const auto& state_chart : state_charts) {
    if (!factory->AddModelFromProto(state_chart)) {
      return nullptr;
    }
  }
  return factory;
}

}  // namespace state_chart

#endif  // STATE_CHART_STATE_MACHINE_FACTORY_H_
