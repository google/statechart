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

#include "statechart/internal/model_builder.h"

#include <vector>

#include "statechart/internal/model_impl.h"
#include "statechart/internal/model/model.h"
#include "statechart/logging.h"
#include "statechart/platform/map_util.h"
#include "statechart/platform/str_util.h"


namespace state_chart {

// static
// TODO(ufirst): change the name of the function at later CL.
Model* ModelBuilder::CreateModelOrNull(const config::StateChart& state_chart) {
  ModelBuilder builder(state_chart);
  builder.Build();
  return builder.CreateModelAndReset();
}

ModelBuilder::ModelBuilder(const config::StateChart& state_chart)
    : state_chart_(state_chart) {}

ModelBuilder::~ModelBuilder() { Reset(); }

void ModelBuilder::Reset() {
  initial_transition_ = nullptr;
  datamodel_block_ = nullptr;
  top_level_states_.clear();
  states_map_.clear();

  // Either the user must call CreateModelAndReset() which will perform memory
  // management by passing all_elements_ to ModelImpl and then clearing the
  // map, or the user never called CreateModelAndReset() and so we have to do it
  // ourselves.
  gtl::STLDeleteElements(&all_elements_);
}

bool ModelBuilder::Build() {
  RETURN_FALSE_IF_MSG(state_chart_.state_size() <= 0,
                      "No states in StateChart.");
  for (const auto& state_config : state_chart_.state()) {
    auto* state = BuildState(state_config);
    RETURN_FALSE_IF(state == nullptr);
    top_level_states_.push_back(state);
  }
  RETURN_FALSE_IF_MSG(top_level_states_.empty(),
                      "No top level states: " << state_chart_.DebugString());
  // Build the initial transition. Default is the first state in the
  // configuration.
  std::vector<const model::State*> initial_targets;
  if (state_chart_.initial_size() == 0) {
    initial_targets.push_back(top_level_states_.front());
  } else {
    for (const string& target : state_chart_.initial()) {
      const auto* target_state = gtl::FindOrNull(states_map_, target);
      RETURN_FALSE_IF_MSG(
          target_state == nullptr,
          "Initial state with ID " << target << " is not a defined state..");
      initial_targets.push_back(*target_state);
    }
  }

  // Note that the transition type does not matter for 'initial_transition_' so
  // it is left as the default (external).
  initial_transition_ = new model::Transition(
      nullptr /* source */, initial_targets, {} /* events */,
      "" /* condition */, false /* is_internal */, nullptr /* executables */);
  RETURN_FALSE_IF(initial_transition_ == nullptr);
  all_elements_.push_back(initial_transition_);

  // After building all possible states, we can now build transitions for them.
  for (const auto& name_state : states_map_) {
    if (!name_state.second->IsFinal()) {
      RETURN_FALSE_IF(!BuildTransitionsForState(name_state.second));
    }
  }
  datamodel_block_ = BuildDataModelBlock(state_chart_.datamodel());
  // TODO(ufirst): Add RETURN_FALSE_IF(!datamodel_block_);
  return true;
}

Model* ModelBuilder::CreateModelAndReset() {
  RETURN_NULL_IF(all_elements_.empty());

  std::vector<const model::State*> const_states(top_level_states_.begin(),
                                           top_level_states_.end());
  auto* model =
      new ModelImpl(state_chart_.name(), initial_transition_, const_states,
                    state_chart_.binding(), datamodel_block_, all_elements_);

  RETURN_NULL_IF(model == nullptr);
  // We passed ownership of everything in all_elements_ to ModelImpl, and thus
  // want to make sure that Reset() does not try to double-free them.
  all_elements_.clear();

  Reset();

  return model;
}

// virtual
model::ExecutableContent* ModelBuilder::BuildExecutableBlock(
    const proto2::RepeatedPtrField<config::ExecutableElement>& elements) {
  // Empty blocks are represented by nullptr.
  // TODO(ufirst): Replace with check: RETURN_NULL_IF(elements.size() == 0)
  if (elements.size() == 0) {
    return nullptr;
  }
  std::vector<const model::ExecutableContent*> executables;
  for (const auto& element : elements) {
    executables.push_back(BuildExecutableContent(element));
  }

  auto* block = new model::ExecutableBlock(executables);
  RETURN_NULL_IF(block == nullptr);
  all_elements_.push_back(block);

  return block;
}

// virtual
model::ExecutableContent* ModelBuilder::BuildExecutableContent(
    const config::ExecutableElement& element) {
  // For validation, we store the new element we create, and continue to check
  // for the rest of the fields, ensuring that no more than one field in
  // ExecutableContent is set.
  model::ExecutableContent* executable_content = nullptr;

  if (element.has_raise()) {
    executable_content = BuildRaise(element.raise());
  }
  if (element.has_log()) {
    RETURN_NULL_IF_MSG(executable_content != nullptr,
                       "ExecutableElement may only have one field set.");
    executable_content = BuildLog(element.log());
  }
  if (element.has_assign()) {
    RETURN_NULL_IF_MSG(executable_content != nullptr,
                       "ExecutableElement may only have one field set.");
    executable_content = BuildAssign(element.assign());
  }
  if (element.has_send()) {
    RETURN_NULL_IF_MSG(executable_content != nullptr,
                       "ExecutableElement may only have one field set.");
    executable_content = BuildSend(element.send());
  }
  if (element.has_if_()) {
    RETURN_NULL_IF_MSG(executable_content != nullptr,
                       "ExecutableElement may only have one field set.");
    executable_content = BuildIf(element.if_());
  }
  if (element.has_foreach()) {
    RETURN_NULL_IF_MSG(executable_content != nullptr,
                       "ExecutableElement may only have one field set.");
    executable_content = BuildForEach(element.foreach());
  }

  RETURN_NULL_IF_MSG(executable_content == nullptr,
                     "ExecutableContent not supported: " <<
                     element.DebugString());
  all_elements_.push_back(executable_content);
  return executable_content;
}

// virtual
model::ExecutableContent* ModelBuilder::BuildDataModelBlock(
    const config::DataModel& datamodel) {
  // TODO(ufirst): Replace with check:
  // RETURN_NULL_IF(datamodel.data_size() == 0);
  if (datamodel.data_size() == 0) {
    return nullptr;
  }

  std::vector<const model::ExecutableContent*> executables;

  for (const auto& data : datamodel.data()) {
    RETURN_NULL_IF(!data.has_id());

    const string expr = data.has_expr() ? data.expr() : data.src();
    auto* model_data = new model::Data(data.id(), expr);
    RETURN_NULL_IF(model_data == nullptr);
    executables.push_back(model_data);
    all_elements_.push_back(executables.back());
  }

  auto* block = new model::ExecutableBlock(executables);
  // TODO(ufirst): Add: RETURN_NULL_IF(block == nullptr);
  all_elements_.push_back(block);
  return block;
}

// virtual
model::Raise* ModelBuilder::BuildRaise(const config::Raise& raise_proto) {
  return new model::Raise(raise_proto.event());
}

// virtual
model::Log* ModelBuilder::BuildLog(const config::Log& log_proto) {
  return new model::Log(log_proto.label(), log_proto.expr());
}

// virtual
model::Assign* ModelBuilder::BuildAssign(const config::Assign& assign_proto) {
  return new model::Assign(assign_proto.location(), assign_proto.expr());
}

// virtual
model::Send* ModelBuilder::BuildSend(const config::Send& send_proto) {
  using model::StrOrExpr;

  // TODO(qplau): Support 'delay' and 'delayexpr'.
  RETURN_NULL_IF(!(
      send_proto.event().empty() || send_proto.eventexpr().empty()));
  RETURN_NULL_IF(!(
      send_proto.target().empty() || send_proto.targetexpr().empty()));
  RETURN_NULL_IF(!(
      send_proto.id().empty() || send_proto.idlocation().empty()));
  RETURN_NULL_IF(!(
      send_proto.type().empty() || send_proto.typeexpr().empty()));
  auto* send =
      new model::Send(StrOrExpr(send_proto.event(), send_proto.eventexpr()),
                      StrOrExpr(send_proto.target(), send_proto.targetexpr()),
                      StrOrExpr(send_proto.id(), send_proto.idlocation()),
                      StrOrExpr(send_proto.type(), send_proto.typeexpr()));
  RETURN_NULL_IF(send == nullptr);

  // Note that the parameters differ from the state chart specification in that
  // for duplicate parameters only the first parameter encountered will be added
  // to Send. The 'namelist' attribute will take precedence over 'param'
  // children.
  for (const string& id : send_proto.namelist()) {
    send->AddParamById(id);
  }
  for (const auto& param : send_proto.param()) {
    RETURN_NULL_IF_MSG(param.has_expr() && param.has_location(),
                       param.DebugString());
    string expr = param.has_expr() ? param.expr() : param.location();
    RETURN_NULL_IF(expr.empty());
    send->AddParamByExpression(param.name(), expr);
  }
  return send;
}

// virtual
model::If* ModelBuilder::BuildIf(const config::If& if_proto) {
  std::vector<std::pair<string, const model::ExecutableContent*>>
      cond_executable;
  for (const auto& cond_config : if_proto.cond_executable()) {
    const auto* executable = BuildExecutableBlock(cond_config.executable());
    // TODO(ufirst): Add check: RETURN_NULL_IF(executable == nullptr);
    cond_executable.push_back(std::make_pair(cond_config.cond(), executable));
  }
  return new model::If(cond_executable);
}

// virtual
model::ForEach* ModelBuilder::BuildForEach(
    const config::ForEach& for_each_proto) {
  return new model::ForEach(for_each_proto.array(),
                            for_each_proto.item(),
                            for_each_proto.index(),
                            BuildExecutableBlock(for_each_proto.executable()));
}

// virtual
model::State* ModelBuilder::BuildState(const config::StateElement& element) {
  model::State* state = nullptr;
  if (element.has_state()) {
    const config::State& config = element.state();
    RETURN_NULL_IF_MSG((config.initial_id_size() > 0 && config.has_initial()),
                       config.DebugString());
    // Initial cannot be specified in atomic states.
    RETURN_NULL_IF_MSG(
        (config.initial_id_size() > 0 && config.state_size() == 0),
        config.DebugString());
    RETURN_NULL_IF_MSG((config.has_initial() && config.state_size() == 0),
        config.DebugString());

    // Build children before parent so that the initial transition can reference
    // them.
    std::vector<model::State*> children;
    for (const auto& child_state : config.state()) {
      auto* child_state_model = BuildState(child_state);
      RETURN_NULL_IF(child_state_model == nullptr);
      children.push_back(child_state_model);
    }

    // Find targets for initial transition. The target must be already created.
    std::vector<const model::State*> targets;
    if (config.initial_id_size() > 0) {
      for (const string& id : config.initial_id()) {
        const auto* initial_id = gtl::FindOrNull(states_map_, id);
        RETURN_NULL_IF_MSG(initial_id == nullptr,
                           "Initial attribute target state with ID  " << id <<
                           " is not a defined state.");
        targets.push_back(*initial_id);
      }
    } else if (config.has_initial()) {
      for (const auto& target : config.initial().transition().target()) {
        const auto* target_state = gtl::FindOrNull(states_map_, target);
        RETURN_NULL_IF_MSG(target_state == nullptr,
                         "Target State ID " << target << " does not exist.");
        targets.push_back(*target_state);
      }
    } else if (!children.empty()) {
      // No initial transition or id and there are children, then the first
      // child is the initial target.
      targets.push_back(children.front());
    }

    state = new model::State(config.id(), false, false,
                             BuildDataModelBlock(config.datamodel()),
                             BuildExecutableBlock(config.onentry()),
                             BuildExecutableBlock(config.onexit()));

    // Create the initial transition if targets exists.
    if (!targets.empty()) {
      auto* initial_transition =
          new model::Transition(state, targets, {}, "", false, nullptr);
      RETURN_NULL_IF(initial_transition == nullptr);
      RETURN_NULL_IF(!state->SetInitialTransition(initial_transition));
      all_elements_.push_back(initial_transition);
    }

    // Add children.
    for (auto child : children) {
      state->AddChild(child);
    }
  } else if (element.has_parallel()) {
    const config::Parallel& config = element.parallel();

    std::vector<model::State*> children;
    for (const auto& child_state : config.state()) {
      auto* child_state_model = BuildState(child_state);
      RETURN_NULL_IF(child_state_model == nullptr);
      children.push_back(child_state_model);
    }

    state = new model::State(config.id(), false, true,
                             BuildDataModelBlock(config.datamodel()),
                             BuildExecutableBlock(config.onentry()),
                             BuildExecutableBlock(config.onexit()));
    RETURN_NULL_IF(state == nullptr);

    // Add children.
    for (auto child : children) {
      state->AddChild(child);
    }

    // Parallel states transit to all children.
    if (!children.empty()) {
      auto* initial_transition = new model::Transition(
          state,
          std::vector<const model::State*>(children.begin(), children.end()),
          {}, "", false, nullptr);
      RETURN_NULL_IF(initial_transition == nullptr);
      RETURN_NULL_IF(!state->SetInitialTransition(initial_transition));
      all_elements_.push_back(initial_transition);
    }
  } else if (element.has_final()) {
    const config::Final& config = element.final();

    // TODO(thatguy): Add donedata support
    state = new model::State(config.id(), true, false, nullptr,
                             BuildExecutableBlock(config.onentry()),
                             BuildExecutableBlock(config.onexit()));
    RETURN_NULL_IF(state == nullptr);
  } else {
    LOG(DFATAL) << "Unimplemented state element:\n" << element.DebugString();
    return nullptr;
  }

  RETURN_NULL_IF(state == nullptr);
  all_elements_.push_back(state);

  RETURN_NULL_IF_MSG(!gtl::InsertIfNotPresent(&states_map_, state->id(), state),
                     "Duplicate state: " << state->id());
  RETURN_NULL_IF_MSG(
      !gtl::InsertIfNotPresent(&states_config_map_, state->id(), &element),
      "Duplicate element: " << element.DebugString());

  return state;
}

// virtual
bool ModelBuilder::BuildTransitionsForState(model::State* state) {
  const auto* config = gtl::FindOrNull(states_config_map_, state->id());
  RETURN_FALSE_IF_MSG(config == nullptr,
                      "State ID " << state->id() << " does not exist.");

  std::vector<const config::Transition*> transitions;
  for (const auto& transition_config : (*config)->state().transition()) {
    transitions.push_back(&transition_config);
  }
  for (const auto& transition_config : (*config)->parallel().transition()) {
    transitions.push_back(&transition_config);
  }

  for (const auto* transition_config : transitions) {
    std::vector<string> events;
    for (const auto& event : transition_config->event()) {
      events.push_back(strings::StripSuffixString(
          strings::StripSuffixString(event, ".*"), "."));
    }

    std::vector<const model::State*> targets;
    for (const auto& target : transition_config->target()) {
      const auto* target_state = gtl::FindOrNull(states_map_, target);
      RETURN_FALSE_IF_MSG(target_state == nullptr,
                          "Target State ID " << target << " does not exist.");
      targets.push_back(*target_state);
    }
    auto* transition = new model::Transition(
        state, targets, events, transition_config->cond(),
        transition_config->type() == config::Transition::TYPE_INTERNAL,
        BuildExecutableBlock(transition_config->executable()));
    RETURN_FALSE_IF(!transition);
    all_elements_.push_back(transition);

    state->mutable_transitions()->push_back(transition);
  }
  return true;
}

}  // namespace state_chart
