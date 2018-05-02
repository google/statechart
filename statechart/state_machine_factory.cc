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

#include "statechart/state_machine_factory.h"

#include <glog/logging.h>

#include "statechart/internal/datamodel.h"
#include "statechart/internal/executor.h"
#include "statechart/internal/function_dispatcher.h"
#include "statechart/internal/light_weight_datamodel.h"
#include "statechart/internal/model.h"
#include "statechart/internal/model_builder.h"
#include "statechart/internal/model_impl.h"
#include "statechart/internal/runtime.h"
#include "statechart/internal/runtime_impl.h"
#include "statechart/internal/state_machine_impl.h"
#include "statechart/internal/state_machine_logger.h"
#include "statechart/logging.h"
#include "statechart/platform/map_util.h"
#include "statechart/proto/state_chart.pb.h"
#include "statechart/proto/state_machine_context.pb.h"
#include "statechart/state_machine.h"

namespace state_chart {

StateMachineFactory::StateMachineFactory()
    : StateMachineFactory(std::unique_ptr<StateMachineListener>(
          ::absl::make_unique<StateMachineLogger>())) {}

StateMachineFactory::StateMachineFactory(
    std::unique_ptr<StateMachineListener> listener)
    : executor_(new Executor()), listener_(std::move(listener)) {}

StateMachineFactory::~StateMachineFactory() {}

bool StateMachineFactory::AddModelFromProto(
    const config::StateChart& state_chart) {
  RETURN_FALSE_IF(state_chart.name().empty());
  const Model* model = ModelBuilder::CreateModelOrNull(state_chart);
  RETURN_FALSE_IF(model == nullptr);
  if (gtl::ContainsKey(models_, model->GetName())) {
    LOG(WARNING) << "Existing model replaced with:" << std::endl
                 << state_chart.DebugString();
  }
  models_[model->GetName()].reset(model);
  return true;
}

std::unique_ptr<StateMachine> StateMachineFactory::CreateStateMachine(
    const string& model_name, FunctionDispatcher* function_dispatcher) const {
  const auto* model = gtl::FindOrNull(models_, model_name);
  RETURN_NULL_IF(model == nullptr || function_dispatcher == nullptr);
  // TODO(qplau): Create datamodel instance based on the model's datamodel type.
  std::unique_ptr<StateMachine> state_machine = StateMachineImpl::Create(
      executor_.get(), model->get(),
      RuntimeImpl::Create(LightWeightDatamodel::Create(function_dispatcher)));
  RETURN_NULL_IF(state_machine == nullptr);
  state_machine->AddListener(listener_.get());
  return state_machine;
}

std::unique_ptr<StateMachine> StateMachineFactory::CreateStateMachine(
    const string& model_name, const StateMachineContext& state_machine_context,
    state_chart::FunctionDispatcher* function_dispatcher) const {
  const auto* model = gtl::FindOrNull(models_, model_name);
  RETURN_NULL_IF(model == nullptr);
  // TODO(qplau): Create datamodel instance based on the model's datamodel type.
  std::unique_ptr<Datamodel> datamodel = LightWeightDatamodel::Create(
      state_machine_context.datamodel(), function_dispatcher);

  // Create Runtime.
  auto runtime = RuntimeImpl::Create(std::move(datamodel));
  const auto& serialized_runtime = state_machine_context.runtime();
  // Set the correct states as active in runtime.
  for (const auto* active_state :
       (*model)->GetActiveStates(serialized_runtime.active_state())) {
    runtime->AddActiveState(active_state);
  }
  runtime->SetRunning(serialized_runtime.running());

  // Create StateMachine.
  std::unique_ptr<StateMachine> state_machine = StateMachineImpl::Create(
      executor_.get(), model->get(), std::move(runtime));
  RETURN_NULL_IF(state_machine == nullptr);
  state_machine->AddListener(listener_.get());
  return state_machine;
}

bool StateMachineFactory::HasModel(const string& model_name) const {
  return gtl::ContainsKey(models_, model_name);
}

}  // namespace state_chart
