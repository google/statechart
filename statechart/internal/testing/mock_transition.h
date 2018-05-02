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

#ifndef STATE_CHART_INTERNAL_TESTING_MOCK_TRANSITION_H_
#define STATE_CHART_INTERNAL_TESTING_MOCK_TRANSITION_H_

#include "statechart/platform/types.h"
#include "statechart/internal/model/transition.h"

#include <gmock/gmock.h>

namespace state_chart {

// Convenience typedef for gmock Return() actions:
// EXPECT_THAT(model_, GetEventlessTransitions(_))
//     .WillOnce(Return(Transitions{&transition1, &transition2}));
typedef std::vector<const model::Transition*> Transitions;

class MockTransition : public model::Transition {
 public:
  MockTransition(const model::State* source,
                 const std::vector<const model::State*>& targets,
                 const std::vector<string>& events,
                 const string& cond_expr = "", bool is_internal = false,
                 const model::ExecutableContent* executable = nullptr)
      : Transition(source, targets, events, cond_expr, is_internal,
                   executable) {
    using testing::_;
    using testing::Return;
    ON_CALL(*this, EvaluateCondition(_)).WillByDefault(Return(true));
  }

  // Create a transition with a single target.
  MockTransition(const model::State* source, const model::State* target,
                 const std::vector<string>& events = {},
                 const string& cond_expr = "",
                 const model::ExecutableContent* executable = nullptr)
      : MockTransition(source, target == nullptr
                                   ? std::vector<const model::State*>{}
                                   : std::vector<const model::State*>{target},
                       events, cond_expr, false, executable) {}

  MockTransition(const model::State* source, const model::State* target,
                 const model::ExecutableContent* executable)
      : MockTransition(source, target, {} /* empty events */, "", executable) {}

  MockTransition(const MockTransition&) = delete;
  MockTransition& operator=(const MockTransition&) = delete;

  MOCK_CONST_METHOD1(EvaluateCondition, bool(Runtime*));
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_TESTING_MOCK_TRANSITION_H_
