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

#ifndef STATE_CHART_INTERNAL_MODEL_IMPL_H_
#define STATE_CHART_INTERNAL_MODEL_IMPL_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "statechart/internal/model.h"
#include "statechart/platform/protobuf.h"
#include "statechart/platform/types.h"
#include "statechart/proto/state_chart.pb.h"
#include "statechart/proto/state_machine_context.pb.h"

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

// Basic model implementation. The model manages the memory of all model related
// objects.
class ModelImpl : public Model {
 public:
  // ModelImpl takes ownership of all objects.
  // Params:
  //  name               The name of the model.
  //  initial_transition The top level transition.
  //  top_level_states   The top level states.
  //  datamodel_binding  Late or early binding.
  //  datamodel          Executable content, i.e., <data> declarations.
  //  model_elements     All model objects to take ownership of.
  ModelImpl(const string& name, const model::Transition* initial_transition,
            const std::vector<const model::State*>& top_level_states,
            config::StateChart::Binding datamodel_binding,
            const model::ExecutableContent* datamodel,
            const std::vector<const model::ModelElement*>& model_elements);

  ModelImpl(const ModelImpl&) = delete;
  ModelImpl& operator=(const ModelImpl&) = delete;

  string GetName() const override { return name_; }

  std::vector<const model::Transition*> GetEventlessTransitions(
      Runtime* runtime) const override {
    return SelectTransitions(runtime, nullptr);
  }

  std::vector<const model::Transition*> GetTransitionsForEvent(
      Runtime* runtime, const string& event) const override {
    return SelectTransitions(runtime, &event);
  }

  const model::Transition* GetInitialTransition() const override {
    return initial_transition_;
  }

  config::StateChart::Binding GetDatamodelBinding() const override {
    return datamodel_binding_;
  }

  const model::ExecutableContent* GetDatamodelBlock() const override {
    return datamodel_;
  }

  std::vector<const model::State*> GetTopLevelStates() const override {
    return top_level_states_;
  }

  // Returns pointers to state(s) for a given tree of active states.
  std::vector<const model::State*> GetActiveStates(
      const proto2::RepeatedPtrField<
          StateMachineContext::Runtime::ActiveStateElement>& active_states)
      const override;

  bool ComputeEntrySet(
      const Runtime* runtime,
      const std::vector<const model::Transition*>& transitions,
      std::vector<const model::State*>* states_to_enter,
      std::set<const model::State*>* states_for_default_entry) const override;

  std::vector<const model::State*> ComputeExitSet(
      const Runtime* runtime,
      const std::vector<const model::Transition*>& transitions) const override;

  void SortStatesByDocumentOrder(
      bool reverse, std::vector<const model::State*>* states) const override;

  bool IsInFinalState(const Runtime* runtime,
                      const model::State* state) const override;

 protected:
  // Returns true if state1 comes before state2 in document order.
  virtual bool StateDocumentOrderLessThan(const model::State* state1,
                                          const model::State* state2) const;

  // Compute the enabled transitions. The event may be nullptr in which case
  // this method computes the eventless transitions.
  virtual std::vector<const model::Transition*> SelectTransitions(
      Runtime* runtime, const string* event) const;

  // The transitions should be in the order of the state that selected them
  // before being passed to this method. See the pseudo-code function
  // 'removeConflictingTransitions()' for an explanation of conflicting
  // transition cases in parallel states.
  // http://www.w3.org/TR/scxml/#AlgorithmforSCXMLInterpretation
  virtual std::vector<const model::Transition*> RemoveConflictingTransitions(
      const Runtime* runtime,
      const std::vector<const model::Transition*>& transitions) const;

 private:
  // Name of the state chart.
  const string name_;
  // The initial transition.
  const model::Transition* initial_transition_;
  // The top-level states sorted by document order.
  std::vector<const model::State*> top_level_states_;
  // Binding type.
  config::StateChart::Binding datamodel_binding_;
  // The data model element.
  const model::ExecutableContent* datamodel_;
  // This contains all ModelElements reacheable from 'top_level_states' for
  // memory management.
  std::vector<std::unique_ptr<const model::ModelElement>> model_elements_;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_MODEL_IMPL_H_
