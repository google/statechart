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

#include "statechart/internal/model/transition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "statechart/internal/testing/mock_runtime.h"

namespace state_chart {
namespace model {
namespace {

using ::testing::NotNull;
using ::testing::_;

TEST(TransitionTest, EvaluateCondition) {
  MockRuntime runtime;

  // Test transition without a condition.
  Transition transition_no_condition(nullptr, {}, {}, "", false, nullptr);
  EXPECT_CALL(runtime.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("", _))
      .Times(0);
  EXPECT_TRUE(transition_no_condition.EvaluateCondition(&runtime));

  // Test transition with a condition that is true.
  Transition transition_with_true(nullptr, {}, {}, "true", false, nullptr);
  EXPECT_CALL(runtime.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("true", NotNull()))
      .WillOnce(ReturnEvaluationResult(true));
  EXPECT_TRUE(transition_with_true.EvaluateCondition(&runtime));

  // Test transition with a condition that is false.
  Transition transition_with_false(nullptr, {}, {}, "false", false, nullptr);
  EXPECT_CALL(runtime.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("false", NotNull()))
      .WillOnce(ReturnEvaluationResult(false));
  EXPECT_FALSE(transition_with_false.EvaluateCondition(&runtime));

  // Test transition with a condition evaluation error.
  Transition transition_with_error(nullptr, {}, {}, "error", false, nullptr);
  EXPECT_CALL(runtime.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("error", NotNull()))
      .WillOnce(ReturnEvaluationError());
  EXPECT_CALL(runtime, EnqueueInternalEvent("error.execution", _));
  EXPECT_FALSE(transition_with_error.EvaluateCondition(&runtime));
}

}  // namespace
}  // namespace model
}  // namespace state_chart
