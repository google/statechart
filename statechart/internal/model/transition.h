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

// IWYU pragma: private, include "src/internal/model/model.h"
// IWYU pragma: friend src/internal/model/*

#ifndef STATE_CHART_INTERNAL_MODEL_TRANSITION_H_
#define STATE_CHART_INTERNAL_MODEL_TRANSITION_H_

#include <string>
#include <vector>

#include "statechart/platform/types.h"
#include "statechart/internal/model/model_element.h"

namespace state_chart {
class Runtime;
namespace model {
class ExecutableContent;
class State;
}  // namespace model
}  // namespace state_chart

namespace state_chart {
namespace model {

// Basic Transition implementation.
class Transition : public ModelElement {
 public:
  // Params:
  //  source      The source state where this transition is nested in, may be
  //              nullptr for transitions of initial elements.
  //  targets     The target states for transition.
  //  events      The list of event descriptors to transit on.
  //  is_internal True if the type of transition is internal, otherwise the type
  //              is external.
  //  executable  Executable content body of the transition.
  Transition(const State* source, const std::vector<const State*>& targets,
             const std::vector<string>& events, const string& cond_expr,
             bool is_internal, const ExecutableContent* executable);

  Transition(const Transition&) = delete;
  Transition& operator=(const Transition&) = delete;

  const State* GetSourceState() const { return source_; }
  const std::vector<const State*>& GetTargetStates() const { return targets_; }
  const std::vector<string>& GetEvents() const { return events_; }
  const string& GetCondition() const { return cond_expr_; }
  bool IsInternal() const { return is_internal_; }
  const ExecutableContent* GetExecutable() const { return executable_; }

  // Evaluates the condition. Returns true if the condition is satisfied or if
  // there is no condition on this transition. Returns false if the condition is
  // unsatisfied or an error occurred while evaluating the condition. An
  // 'error.execution' event will be enqueued in 'runtime' when evaluation
  // fails.
  virtual bool EvaluateCondition(Runtime* runtime) const;

  // A string for representing the transition.
  string DebugString() const;

 private:
  const State* source_;
  const std::vector<const State*> targets_;
  const std::vector<string> events_;
  // The condition expression, empty string for none.
  const string cond_expr_;
  const bool is_internal_;
  const ExecutableContent* executable_;
};

}  // namespace model
}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_MODEL_TRANSITION_H_
