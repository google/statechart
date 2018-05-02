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

#include "statechart/internal/state_machine_impl.h"

#include "absl/memory/memory.h"
#include "statechart/internal/event_dispatcher.h"
#include "statechart/internal/executor.h"
#include "statechart/internal/light_weight_datamodel.h"
#include "statechart/internal/model.h"
#include "statechart/internal/runtime.h"

using ::absl::WrapUnique;

namespace state_chart {
class StateMachineListener;
}  // namespace state_chart

namespace state_chart {

// static
std::unique_ptr<StateMachine> StateMachineImpl::Create(
    const Executor* executor, const Model* model,
    std::unique_ptr<Runtime> runtime) {
  RETURN_NULL_IF(executor == nullptr || model == nullptr || runtime == nullptr);
  return WrapUnique(new StateMachineImpl(executor, model, std::move(runtime)));
}

StateMachineImpl::StateMachineImpl(const Executor* executor,
                                   const Model* model,
                                   std::unique_ptr<Runtime> runtime)
    : executor_(executor), model_(model), runtime_(std::move(runtime)) {}

// override
void StateMachineImpl::Start() {
  executor_->Start(model_, runtime_.get());
}

// override
void StateMachineImpl::SendEvent(const string& event, const string& payload) {
  executor_->SendEvent(model_, runtime_.get(), event, payload);
}

// override
void StateMachineImpl::AddListener(StateMachineListener* listener) {
  runtime_->GetEventDispatcher()->AddListener(listener);
}

// override
const Runtime& StateMachineImpl::GetRuntime() const {
  return *runtime_;
}

// override
const Model& StateMachineImpl::GetModel() const {
  return *model_;
}

}  // namespace state_chart
