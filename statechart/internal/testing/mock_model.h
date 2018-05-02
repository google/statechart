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

#ifndef STATE_CHART_INTERNAL_TESTING_MOCK_MODEL_H_
#define STATE_CHART_INTERNAL_TESTING_MOCK_MODEL_H_

#include <iterator>
#include <memory>
#include <vector>

#include "absl/algorithm/container.h"
#include "statechart/internal/model.h"
#include "statechart/internal/testing/mock_transition.h"
#include "statechart/platform/types.h"

#include <gmock/gmock.h>

namespace state_chart {
class Runtime;
namespace model {
class ExecutableContent;
class ModelElement;
class State;
class Transition;
}  // namespace model
}  // namespace state_chart

namespace state_chart {

// Set arg2 to be the target states of the transitions in arg1.
ACTION(ReturnTransitionTargetStates) {
  for (const auto* transition : arg1) {
    absl::c_copy(transition->GetTargetStates(), back_inserter(*arg2));
  }
  return true;
}

// Set arg2 to be the source states of the transitions in arg1 if they exists.
ACTION(ReturnTransitionSourceStates) {
  std::vector<const model::State*> states;
  for (const auto* transition : arg1) {
    if (transition->GetSourceState() != nullptr) {
      states.push_back(transition->GetSourceState());
    }
  }
  return states;
}

// CHECK-fail if arg1 pointee is vector of size > 1. For use with
// SortStatesByDocumentOrder.
ACTION(CheckFailIfArg1PointeeSizeGreaterThanOne) {
  CHECK_LE(arg1->size(), 1u) << "SortStatesByDocumentOrder expectations must be "
                               "set if sorting > 1 elements.";
}

class MockModel : public Model {
 public:
  explicit MockModel(config::StateChart::Binding datamodel_binding =
                         config::StateChart::BINDING_EARLY,
                     const model::ExecutableContent* datamodel = nullptr)
      : datamodel_binding_(datamodel_binding), datamodel_(datamodel) {
    using ::testing::_;
    using ::testing::Return;

    ON_CALL(*this, GetEventlessTransitions(_))
        .WillByDefault(Return(Transitions()));
    ON_CALL(*this, ComputeEntrySet(_, _, _, _))
        .WillByDefault(ReturnTransitionTargetStates());
    ON_CALL(*this, ComputeExitSet(_, _))
        .WillByDefault(ReturnTransitionSourceStates());
    ON_CALL(*this, SortStatesByDocumentOrder(_, _))
        .WillByDefault(CheckFailIfArg1PointeeSizeGreaterThanOne());
  }
  MockModel(const MockModel&) = delete;
  MockModel& operator=(const MockModel&) = delete;
  ~MockModel() override {}

  // Create the initial transition for a given state target.
  void CreateInitialTransition(const model::State* target) {
    initial_transition_.reset(new MockTransition(nullptr, target));
  }

  string GetName() const override { return "mock"; }

  MOCK_CONST_METHOD1(GetEventlessTransitions,
                     std::vector<const model::Transition*>(Runtime*));
  MOCK_CONST_METHOD2(GetTransitionsForEvent,
                     std::vector<const model::Transition*>(Runtime*,
                                                           const string&));

  const model::Transition* GetInitialTransition() const override {
    return initial_transition_.get();
  }

  config::StateChart::Binding GetDatamodelBinding() const override {
    return datamodel_binding_;
  }

  void SetDatamodelBlock(const model::ExecutableContent* datamodel) {
    datamodel_ = datamodel;
  }

  const model::ExecutableContent* GetDatamodelBlock() const override {
    return datamodel_;
  }

  MOCK_CONST_METHOD0(GetTopLevelStates, std::vector<const model::State*>());

  MOCK_CONST_METHOD4(
      ComputeEntrySet,
      bool(const Runtime* runtime,
           const std::vector<const model::Transition*>& transitions,
           std::vector<const model::State*>* states_to_enter,
           std::set<const model::State*>* states_for_default_entry));

  MOCK_CONST_METHOD2(
      ComputeExitSet,
      std::vector<const model::State*>(
          const Runtime* runtime,
          const std::vector<const model::Transition*>& transitions));

  MOCK_CONST_METHOD2(SortStatesByDocumentOrder,
                     void(bool reverse,
                          std::vector<const model::State*>* states));

  MOCK_CONST_METHOD2(IsInFinalState, bool(const Runtime*, const model::State*));

  MOCK_CONST_METHOD1(GetActiveStates,
                     std::vector<const model::State*>(
                         const proto2::RepeatedPtrField<
                             StateMachineContext::Runtime::ActiveStateElement>&
                             active_states));

 private:
  std::unique_ptr<const model::Transition> initial_transition_;
  config::StateChart::Binding datamodel_binding_;
  const model::ExecutableContent* datamodel_;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_TESTING_MOCK_MODEL_H_
