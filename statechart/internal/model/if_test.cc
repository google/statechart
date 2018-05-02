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

#include "statechart/internal/model/if.h"

#include <memory>

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "statechart/internal/testing/mock_datamodel.h"
#include "statechart/internal/testing/mock_executable_content.h"
#include "statechart/internal/testing/mock_runtime.h"



namespace state_chart {
namespace model {
namespace {

using ::testing::_;

class IfTest : public testing::Test {
 protected:
  MockExecutableContent* AddCondition(const string& expr) {
    executable_content_storage_.emplace_back(new MockExecutableContent());
    auto* executable = executable_content_storage_.back().get();

    condition_executable_.push_back(std::make_pair(expr, executable));
    return executable;
  }

  bool Execute() {
    if_.reset(new If(condition_executable_));
    return if_->Execute(&runtime_);
  }

  std::unique_ptr<If> if_;
  MockRuntime runtime_;

  std::vector<std::unique_ptr<MockExecutableContent>>
      executable_content_storage_;
  std::vector<std::pair<string, const ExecutableContent*>>
      condition_executable_;
};

TEST_F(IfTest, BasicIf_False) {
  auto* executable = AddCondition("cond1");

  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond1", _))
      .WillOnce(ReturnEvaluationResult(false));

  EXPECT_CALL(*executable, Execute(_)).Times(0);

  EXPECT_TRUE(Execute());
}

TEST_F(IfTest, BasicIf_True) {
  auto* executable = AddCondition("cond1");

  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond1", _))
      .WillOnce(ReturnEvaluationResult(true));

  EXPECT_CALL(*executable, Execute(&runtime_));

  EXPECT_TRUE(Execute());
}

TEST_F(IfTest, IfElse_IfTrue) {
  // First show that if 'if' succeeds, 'else' is not called.
  auto* if_executable = AddCondition("if");
  auto* else_executable = AddCondition("");

  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("if", _))
      .WillOnce(ReturnEvaluationResult(true));

  EXPECT_CALL(*if_executable, Execute(&runtime_));
  EXPECT_CALL(*else_executable, Execute(&runtime_)).Times(0);

  EXPECT_TRUE(Execute());
}

TEST_F(IfTest, IfElse_ElseTrue) {
  // If 'if' fails, 'else' should fire.
  auto* if_executable = AddCondition("if");
  auto* else_executable = AddCondition("");

  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("if", _))
      .WillOnce(ReturnEvaluationResult(false));

  EXPECT_CALL(*if_executable, Execute(&runtime_)).Times(0);
  EXPECT_CALL(*else_executable, Execute(&runtime_));

  Execute();
}

TEST_F(IfTest, LongIfElseIfElseChain) {
  // Create a chain of if/elseif blocks and show that only the first condition
  // that evaluates to true is executed, and the rest are short-circuited.
  auto* executable1 = AddCondition("cond1");
  auto* executable2 = AddCondition("cond2");
  auto* executable3 = AddCondition("cond3");
  auto* executable4 = AddCondition("cond4");
  auto* executable5 = AddCondition("cond5");

  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond1", _))
      .WillOnce(ReturnEvaluationResult(false));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond2", _))
      .WillOnce(ReturnEvaluationResult(false));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond3", _))
      .WillOnce(ReturnEvaluationResult(false));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond4", _))
      .WillOnce(ReturnEvaluationResult(true));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond5", _)).Times(0);

  EXPECT_CALL(*executable1, Execute(&runtime_)).Times(0);
  EXPECT_CALL(*executable2, Execute(&runtime_)).Times(0);
  EXPECT_CALL(*executable3, Execute(&runtime_)).Times(0);
  EXPECT_CALL(*executable4, Execute(&runtime_));
  EXPECT_CALL(*executable5, Execute(&runtime_)).Times(0);

  EXPECT_TRUE(Execute());
}

TEST_F(IfTest, EvaluationError) {
  // If one of the evaluations results in an error, we should see an internal
  // event queued of type 'error.execution', that conditional should evaluate to
  // 'false', and then evaluation of the remaining conditionals should continue.
  auto* executable1 = AddCondition("cond1");
  auto* executable2 = AddCondition("cond2");
  auto* executable3 = AddCondition("cond3");
  auto* executable4 = AddCondition("cond4");
  auto* executable5 = AddCondition("cond5");

  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond1", _))
      .WillOnce(ReturnEvaluationResult(false));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond2", _))
      .WillOnce(ReturnEvaluationResult(false));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond3", _))
      .WillOnce(ReturnEvaluationResult(false));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond4", _))
      .WillOnce(ReturnEvaluationError());
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond5", _))
      .WillOnce(ReturnEvaluationResult(true));

  EXPECT_CALL(*executable1, Execute(&runtime_)).Times(0);
  EXPECT_CALL(*executable2, Execute(&runtime_)).Times(0);
  EXPECT_CALL(*executable3, Execute(&runtime_)).Times(0);
  EXPECT_CALL(*executable4, Execute(&runtime_)).Times(0);
  EXPECT_CALL(*executable5, Execute(&runtime_)).Times(1);

  EXPECT_CALL(runtime_, EnqueueInternalEvent("error.execution", _));

  EXPECT_FALSE(Execute());
}

}  // namespace
}  // namespace model
}  // namespace state_chart
