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

#include "statechart/internal/model/data.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "statechart/internal/testing/mock_datamodel.h"
#include "statechart/internal/testing/mock_runtime.h"

namespace state_chart {
namespace model {
namespace {

using ::testing::_;
using ::testing::Return;

TEST(DataTest, EvaluationError) {
  Data data("location", "expression");

  MockRuntime runtime;
  EXPECT_CALL(runtime.GetDefaultMockDatamodel(), Declare("location"))
      .WillOnce(Return(false));
  EXPECT_CALL(runtime, EnqueueInternalEvent("error.execution", _));
  EXPECT_FALSE(data.Execute(&runtime));

  EXPECT_CALL(runtime.GetDefaultMockDatamodel(), Declare("location"))
        .WillOnce(Return(true));
  EXPECT_CALL(runtime.GetDefaultMockDatamodel(),
              AssignExpression("location", "expression"))
      .WillOnce(Return(false));
  EXPECT_CALL(runtime, EnqueueInternalEvent("error.execution", _));
  EXPECT_FALSE(data.Execute(&runtime));
}

TEST(DataTest, ValidDeclaration) {
  Data data("location", "expression");

  MockRuntime runtime;
  EXPECT_CALL(runtime.GetDefaultMockDatamodel(), Declare("location"))
          .WillOnce(Return(true));
  EXPECT_CALL(runtime.GetDefaultMockDatamodel(),
              AssignExpression("location", "expression"))
      .WillOnce(Return(true));
  EXPECT_TRUE(data.Execute(&runtime));
}

}  // namespace
}  // namespace model
}  // namespace state_chart
