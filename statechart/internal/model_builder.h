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

#ifndef STATE_CHART_INTERNAL_MODEL_BUILDER_H_
#define STATE_CHART_INTERNAL_MODEL_BUILDER_H_

#include <map>
#include <vector>

#include "statechart/platform/protobuf.h"
#include "statechart/platform/types.h"
#include "statechart/proto/state_chart.pb.h"

namespace state_chart {

class Model;

namespace model {
class Assign;
class ExecutableContent;
class ForEach;
class If;
class Log;
class ModelElement;
class Raise;
class Send;
class State;
class Transition;
}  // namespace model

// This class builds a Model from a StateChart proto, and performs validation
// that the StateChart is well formed. The Model can be used by a StateMachine
// instance, and describes the semantics of a specific state machine.
//
// Typical usage:
//
//   const StateChart my_config = ...;
//
//   ModelBuilder builder(my_config);
//   builder.Build();
//
//   Model* my_model = builder.CreateModelAndReset();
//
// TODO(thatguy): Add error checking ability after Build().
class ModelBuilder {
 public:
  // Returns a new model instance. Caller takes ownership.
  // Returns nullptr if error occurred when building model.
  static Model* CreateModelOrNull(const config::StateChart& state_chart);

  explicit ModelBuilder(const config::StateChart& state_chart);
  virtual ~ModelBuilder();

  // Parses and validates 'state_chart' and creates all model objects. Must be
  // called before CreateModelAndReset().
  bool Build();

  // Returns a new Model instance. Caller takes ownership.
  Model* CreateModelAndReset();

 protected:
  // Returns the internal state of this object to that at instantiation time.
  void Reset();

  // Build various instances of ExecutableContent.

  // Returns nullptr when 'elements.size() == 0', i.e., empty executable blocks
  // are denoted by nullptr.
  virtual model::ExecutableContent* BuildExecutableBlock(
      const proto2::RepeatedPtrField<config::ExecutableElement>& elements);

  virtual model::ExecutableContent* BuildExecutableContent(
      const config::ExecutableElement& element);

  virtual model::ExecutableContent* BuildDataModelBlock(
      const config::DataModel& datamodel);

  virtual model::Raise* BuildRaise(const config::Raise& raise_proto);
  virtual model::Log* BuildLog(const config::Log& log_proto);
  virtual model::Assign* BuildAssign(const config::Assign& assign_proto);
  virtual model::Send* BuildSend(const config::Send& send_proto);
  virtual model::If* BuildIf(const config::If& if_proto);
  virtual model::ForEach* BuildForEach(const config::ForEach& for_each_proto);

  // Build State-ful objects.
  virtual model::State* BuildState(const config::StateElement& state_proto);

  // Transitions.
  virtual bool BuildTransitionsForState(model::State* state);

  // Member variables.
  const config::StateChart state_chart_;

  const model::Transition* initial_transition_ = nullptr;
  model::ExecutableContent* datamodel_block_ = nullptr;
  std::vector<model::State*> top_level_states_;

  std::map<string, model::State*> states_map_;
  std::map<string, const config::StateElement*> states_config_map_;
  std::vector<const model::ModelElement*> all_elements_;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_MODEL_BUILDER_H_
