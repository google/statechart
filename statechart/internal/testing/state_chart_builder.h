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

#ifndef STATE_CHART_INTERNAL_TESTING_STATE_CHART_BUILDER_H_
#define STATE_CHART_INTERNAL_TESTING_STATE_CHART_BUILDER_H_

#include <string>

#include <glog/logging.h>

#include "statechart/platform/protobuf.h"
#include "statechart/platform/types.h"
#include "statechart/proto/state_chart.pb.h"

#define SET_IF_NOT_EMPTY(var, attr) \
  if (!attr.empty()) { \
    var->set_ ## attr(attr); \
  }

namespace state_chart {
namespace config {

class ExecutableBlockBuilder {
 public:
  explicit ExecutableBlockBuilder(
      proto2::RepeatedPtrField<ExecutableElement>* block)
      : block_(block), block_stack_(1, block) {}
  ~ExecutableBlockBuilder() {
    CHECK(element_stack_.empty());
    CHECK(if_stack_.empty());
    CHECK(foreach_stack_.empty());
    CHECK_EQ(block_stack_.size(), 1u);
  }

  ExecutableBlockBuilder& AddRaise(const string& event) {
    block_->Add()->mutable_raise()->set_event(event);
    return *this;
  }

  ExecutableBlockBuilder& AddLog(const string& label,
                                 const string& expr) {
    Log* log = block_->Add()->mutable_log();
    log->set_label(label);
    log->set_expr(expr);
    return *this;
  }

  ExecutableBlockBuilder& AddAssign(const string& location,
                                    const string& expr) {
    Assign* assign = block_->Add()->mutable_assign();
    assign->set_location(location);
    assign->set_expr(expr);
    return *this;
  }

  ExecutableBlockBuilder& AddScript(const string& src,
                                    const string& content) {
    Script* script = block_->Add()->mutable_script();
    SET_IF_NOT_EMPTY(script, src);
    SET_IF_NOT_EMPTY(script, content);
    return *this;
  }

  ExecutableBlockBuilder& AddIf(const string& cond) {
    If* if_el = block_->Add()->mutable_if_();
    PushElement(&if_stack_, if_el);

    If::ConditionalExecutable* cond_executable = if_el->add_cond_executable();
    cond_executable->set_cond(cond);

    PushBlock(cond_executable->mutable_executable());
    return *this;
  }

  ExecutableBlockBuilder& ElseIf(const string& cond) {
    PopBlock();

    If::ConditionalExecutable* cond_executable =
        if_stack_.back()->add_cond_executable();
    if (!cond.empty()) {
      cond_executable->set_cond(cond);
    }

    PushBlock(cond_executable->mutable_executable());
    return *this;
  }

  ExecutableBlockBuilder& Else() {
    return ElseIf("");
  }

  ExecutableBlockBuilder& EndIf() {
    PopElement(&if_stack_);
    PopBlock();
    return *this;
  }

  ExecutableBlockBuilder& AddForEach(const string& array,
                                     const string& item,
                                     const string& index) {
    ForEach* for_each = block_->Add()->mutable_foreach();
    PushElement(&foreach_stack_, for_each);

    for_each->set_array(array);
    for_each->set_item(item);
    for_each->set_index(index);

    PushBlock(for_each->mutable_executable());
    return *this;
  }

  ExecutableBlockBuilder& EndForEach() {
    PopElement(&foreach_stack_);
    PopBlock();
    return *this;
  }

  ExecutableBlockBuilder& AddSend(const string& event,
                                  const string& eventexpr,
                                  const string& target,
                                  const string& targetexpr,
                                  const string& type,
                                  const string& typeexpr,
                                  const string& id,
                                  const string& idlocation,
                                  const string& delay,
                                  const string& delayexpr,
                                  const std::vector<string>& namelist) {
    Send* send = block_->Add()->mutable_send();
    SET_IF_NOT_EMPTY(send, event);
    SET_IF_NOT_EMPTY(send, eventexpr);
    SET_IF_NOT_EMPTY(send, target);
    SET_IF_NOT_EMPTY(send, targetexpr);
    SET_IF_NOT_EMPTY(send, type);
    SET_IF_NOT_EMPTY(send, typeexpr);
    SET_IF_NOT_EMPTY(send, id);
    SET_IF_NOT_EMPTY(send, idlocation);
    SET_IF_NOT_EMPTY(send, delay);
    SET_IF_NOT_EMPTY(send, delayexpr);
    for (const string& name : namelist) {
      send->add_namelist(name);
    }

    current_param_ = send->mutable_param();
    current_content_ = send->mutable_content();
    return *this;
  }

  // Modifiers for attributes of elements that are not easy to specify by
  // function parameters.
  ExecutableBlockBuilder& AddParam(const string& name,
                                   const string& expr,
                                   const string& location) {
    CHECK(current_param_ != nullptr);
    Param* param = current_param_->Add();
    param->set_name(name);
    param->set_expr(expr);
    param->set_location(location);
    return *this;
  }

  ExecutableBlockBuilder& SetContentExpr(const string& expr) {
    CHECK(current_content_ != nullptr);
    current_content_->set_expr(expr);
    return *this;
  }

  ExecutableBlockBuilder& SetContentString(const string& content) {
    CHECK(current_content_ != nullptr);
    current_content_->set_content(content);
    return *this;
  }

  ExecutableBlockBuilder& SetContentStateChart(const StateChart& sc) {
    CHECK(current_content_ != nullptr);
    *current_content_->mutable_state_chart() = sc;
    return *this;
  }

 private:
  void PushBlock(proto2::RepeatedPtrField<ExecutableElement>* block) {
    block_stack_.push_back(block);
    block_ = block;
  }

  void PopBlock() {
    block_stack_.pop_back();
    CHECK(!block_stack_.empty());
    block_ = block_stack_.back();
  }

  template <class T>
  void PushElement(std::vector<T*>* stack, T* element) {
    stack->push_back(element);
    element_stack_.push_back(element);
  }

  template <class T>
  void PopElement(std::vector<T*>* stack) {
    CHECK_EQ(stack->back(), element_stack_.back());
    stack->pop_back();
    element_stack_.pop_back();
  }

  std::vector<void*> element_stack_;
  std::vector<If*> if_stack_;
  std::vector<ForEach*> foreach_stack_;

  proto2::RepeatedPtrField<Param>* current_param_ = nullptr;
  Content* current_content_ = nullptr;

  proto2::RepeatedPtrField<ExecutableElement>* block_;
  std::vector<proto2::RepeatedPtrField<ExecutableElement>*> block_stack_;
};

class DataModelBuilder {
 public:
  explicit DataModelBuilder(DataModel* datamodel)
      : datamodel_(datamodel) {}

  DataModelBuilder& AddDataFromExpr(const string& id, const string& expr) {
    Data* d = datamodel_->add_data();
    d->set_id(id);
    d->set_expr(expr);
    return *this;
  }

  DataModelBuilder& AddDataFromSrc(const string& id, const string& src) {
    Data* d = datamodel_->add_data();
    d->set_id(id);
    d->set_src(src);
    return *this;
  }

 private:
  DataModel* datamodel_;
};

ExecutableBlockBuilder BuildTransition(
    Transition* transition,
    const std::vector<string>& events,
    const std::vector<string>& targets,
    const string& cond,
    Transition::Type type) {
  for (const auto& e : events) { transition->add_event(e); }
  if (!cond.empty()) { transition->set_cond(cond); }
  for (const auto& t : targets) { transition->add_target(t); }
  if (type != Transition::TYPE_EXTERNAL) { transition->set_type(type); }

  return ExecutableBlockBuilder(transition->mutable_executable());
}

class StateBuilder {
 public:
  StateBuilder(State* state, const string& id)
      : state_(state) {
    state_->set_id(id);
  }

  StateBuilder& AddInitialId(const string& initial) {
    state_->add_initial_id(initial);
    return *this;
  }

  DataModelBuilder DataModel() {
    return DataModelBuilder(state_->mutable_datamodel());
  }

  ExecutableBlockBuilder OnEntry() {
    return ExecutableBlockBuilder(state_->mutable_onentry());
  }

  ExecutableBlockBuilder OnExit() {
    return ExecutableBlockBuilder(state_->mutable_onexit());
  }

  ExecutableBlockBuilder SetInitialTransition(const string& target) {
    return BuildTransition(state_->mutable_initial()->mutable_transition(),
                           {}, {target}, "", Transition::TYPE_EXTERNAL);
  }

  ExecutableBlockBuilder AddTransition(
      const std::vector<string>& events,
      const std::vector<string>& targets,
      const string& cond = "",
      Transition::Type type = Transition::TYPE_EXTERNAL) {
    return BuildTransition(state_->add_transition(),
                           events, targets, cond, type);
  }

  // Add a child state.
  StateBuilder AddState(const string& id) {
    return StateBuilder(state_->add_state()->mutable_state(), id);
  }

 private:
  State* state_;
};

class ParallelBuilder {
 public:
  ParallelBuilder(Parallel* state, const string& id)
      : state_(state) {
    state_->set_id(id);
  }

  DataModelBuilder DataModel() {
    return DataModelBuilder(state_->mutable_datamodel());
  }

  ExecutableBlockBuilder OnEntry() {
    return ExecutableBlockBuilder(state_->mutable_onentry());
  }

  ExecutableBlockBuilder OnExit() {
    return ExecutableBlockBuilder(state_->mutable_onexit());
  }

  ExecutableBlockBuilder AddTransition(
      const std::vector<string>& events,
      const std::vector<string>& targets,
      const string& cond = "",
      Transition::Type type = Transition::TYPE_EXTERNAL) {
    return BuildTransition(state_->add_transition(),
                           events, targets, cond, type);
  }

  // Add a child state.
  StateBuilder AddState(const string& id) {
    return StateBuilder(state_->add_state()->mutable_state(), id);
  }

 private:
  Parallel* state_;
};

class FinalStateBuilder {
 public:
  // TODO(thatguy): Add donedata support.
  FinalStateBuilder(Final* state, const string& id) : state_(state) {
    state_->set_id(id);
  }

  ExecutableBlockBuilder OnEntry() {
    return ExecutableBlockBuilder(state_->mutable_onentry());
  }

  ExecutableBlockBuilder OnExit() {
    return ExecutableBlockBuilder(state_->mutable_onexit());
  }

 private:
  Final* state_;
};

class StateChartBuilder {
 public:
  StateChartBuilder(StateChart* state_chart, const string& name)
      : state_chart_(state_chart) {
    state_chart_->set_name(name);
  }

  StateChartBuilder& AddInitial(const string& initial) {
    state_chart_->add_initial(initial);
    return *this;
  }

  StateChartBuilder& SetDataModelType(const string& type) {
    state_chart_->set_datamodel_type(type);
    return *this;
  }

  StateChartBuilder& SetBinding(StateChart::Binding binding) {
    state_chart_->set_binding(binding);
    return *this;
  }

  DataModelBuilder DataModel() {
    return DataModelBuilder(state_chart_->mutable_datamodel());
  }

  StateBuilder AddState(const string& id) {
    return StateBuilder(state_chart_->add_state()->mutable_state(), id);
  }

  FinalStateBuilder AddFinalState(const string& id) {
    return FinalStateBuilder(state_chart_->add_state()->mutable_final(), id);
  }

 private:
  StateChart* state_chart_;
};

}  // namespace config
}  // namespace state_chart

#undef SET_IF_NOT_EMPTY

#endif  // STATE_CHART_INTERNAL_TESTING_STATE_CHART_BUILDER_H_
