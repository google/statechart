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

#include "statechart/internal/model/for_each.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "statechart/internal/testing/mock_datamodel.h"
#include "statechart/internal/testing/mock_executable_content.h"
#include "statechart/internal/testing/mock_runtime.h"

namespace state_chart {
namespace model {
namespace {

using ::testing::_;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

TEST(ForEach, LoopWithIndexAndItem) {
  NiceMock<MockRuntime> mock_runtime;
  auto& mock_datamodel = mock_runtime.GetDefaultMockDatamodel();
  NiceMock<MockExecutableContent> mock_loop_body;

  ON_CALL(mock_datamodel, IsDefined(_)).WillByDefault(Return(false));
  ON_CALL(mock_datamodel, Declare(_)).WillByDefault(Return(true));

  ForEach for_each("[0, 2, 4]", "item", "index", &mock_loop_body);

  // No errors allowed.
  EXPECT_CALL(mock_runtime, EnqueueInternalEvent("error.execution", _))
      .Times(0);

  // Result owned by 'for_each'.
  EXPECT_CALL(mock_datamodel, EvaluateIterator("[0, 2, 4]"))
      .WillOnce(Return(new MockIterator({"0", "2", "4"})));

  // Value assigned in right sequence.
  {
    InSequence sequence;
    EXPECT_CALL(mock_datamodel, AssignExpression("item", "0"))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_datamodel, AssignExpression("item", "2"))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_datamodel, AssignExpression("item", "4"))
        .WillOnce(Return(true));
  }

  // Index assigned in right sequence.
  {
    InSequence sequence;
    EXPECT_CALL(mock_datamodel, AssignExpression("index", "0"))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_datamodel, AssignExpression("index", "1"))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_datamodel, AssignExpression("index", "2"))
        .WillOnce(Return(true));
  }

  // Body called thrice.
  EXPECT_CALL(mock_loop_body, Execute(&mock_runtime)).Times(3).WillRepeatedly(
      Return(true));

  EXPECT_TRUE(for_each.Execute(&mock_runtime));
}

TEST(ForEach, LoopWithNoBody) {
  NiceMock<MockRuntime> mock_runtime;
  auto& mock_datamodel = mock_runtime.GetDefaultMockDatamodel();

  ON_CALL(mock_datamodel, IsDefined(_)).WillByDefault(Return(false));
  ON_CALL(mock_datamodel, Declare(_)).WillByDefault(Return(true));

  // Empty body is nullptr.
  ForEach for_each("[1, 0]", "item", "index", nullptr);

  // No errors allowed.
  EXPECT_CALL(mock_runtime, EnqueueInternalEvent("error.execution", _))
      .Times(0);

  // Result owned by 'for_each'.
  EXPECT_CALL(mock_datamodel, EvaluateIterator("[1, 0]"))
      .WillOnce(Return(new MockIterator({"1", "0"})));

  // Value assigned in right sequence.
  {
    InSequence sequence;
    EXPECT_CALL(mock_datamodel, AssignExpression("item", "1"))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_datamodel, AssignExpression("item", "0"))
        .WillOnce(Return(true));
  }

  // Index assigned in right sequence.
  {
    InSequence sequence;
    EXPECT_CALL(mock_datamodel, AssignExpression("index", "0"))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_datamodel, AssignExpression("index", "1"))
        .WillOnce(Return(true));
  }

  // No body calls.
  EXPECT_TRUE(for_each.Execute(&mock_runtime));
}

// Looping without an index variable.
TEST(ForEach, LoopWithNoIndex) {
  NiceMock<MockRuntime> mock_runtime;
  auto& mock_datamodel = mock_runtime.GetDefaultMockDatamodel();
  NiceMock<MockExecutableContent> mock_loop_body;

  ON_CALL(mock_datamodel, IsDefined(_)).WillByDefault(Return(false));
  ON_CALL(mock_datamodel, Declare(_)).WillByDefault(Return(true));

  ForEach for_each("[0, 2, 4]", "item", "", &mock_loop_body);

  // No errors allowed.
  EXPECT_CALL(mock_runtime, EnqueueInternalEvent("error.execution", _))
      .Times(0);

  // Result owned by 'for_each'.
  EXPECT_CALL(mock_datamodel, EvaluateIterator("[0, 2, 4]"))
      .WillOnce(Return(new MockIterator({"0", "2", "4"})));

  // Value assigned in right sequence.
  {
    InSequence sequence;
    EXPECT_CALL(mock_datamodel, AssignExpression("item", "0"))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_datamodel, AssignExpression("item", "2"))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_datamodel, AssignExpression("item", "4"))
        .WillOnce(Return(true));
  }

  // No index assignment.
  EXPECT_CALL(mock_datamodel, AssignExpression("index", _)).Times(0);

  // Body called thrice.
  EXPECT_CALL(mock_loop_body, Execute(&mock_runtime)).Times(3).WillRepeatedly(
      Return(true));

  EXPECT_TRUE(for_each.Execute(&mock_runtime));
}

// Looping on non-existent collection.
TEST(ForEach, LoopOverIllegalCollectionIsError) {
  NiceMock<MockRuntime> mock_runtime;
  auto& mock_datamodel = mock_runtime.GetDefaultMockDatamodel();
  NiceMock<MockExecutableContent> mock_loop_body;

  ON_CALL(mock_datamodel, IsDefined(_)).WillByDefault(Return(false));
  ON_CALL(mock_datamodel, Declare(_)).WillByDefault(Return(true));

  // Illegal collection.
  EXPECT_CALL(mock_datamodel, EvaluateIterator("foo"))
      .WillOnce(Return(nullptr));

  ForEach for_each("foo", "item", "", &mock_loop_body);

  // No declarations if the collection is illegal.
  EXPECT_CALL(mock_datamodel, Declare(_)).Times(0);
  // No assignments.
  EXPECT_CALL(mock_datamodel, AssignExpression(_, _)).Times(0);
  // Loop body never called.
  EXPECT_CALL(mock_loop_body, Execute(&mock_runtime)).Times(0);

  // Single error.
  EXPECT_CALL(mock_runtime, EnqueueInternalEvent("error.execution", _));

  EXPECT_FALSE(for_each.Execute(&mock_runtime));
}

// Error declaring the item variable.
TEST(ForEach, IllegalItemLocationIsError) {
  NiceMock<MockRuntime> mock_runtime;
  auto& mock_datamodel = mock_runtime.GetDefaultMockDatamodel();
  NiceMock<MockExecutableContent> mock_loop_body;

  ON_CALL(mock_datamodel, IsDefined(_)).WillByDefault(Return(false));

  ForEach for_each("[0, 2, 4]", "item", "index", &mock_loop_body);

  // Result owned by 'for_each'.
  EXPECT_CALL(mock_datamodel, EvaluateIterator("[0, 2, 4]"))
      .WillOnce(Return(new MockIterator({"0", "2", "4"})));

  // Illegal declaration.
  EXPECT_CALL(mock_datamodel, Declare("item")).WillOnce(Return(false));

  // No assignments.
  EXPECT_CALL(mock_datamodel, AssignExpression(_, _)).Times(0);
  // Loop body never called.
  EXPECT_CALL(mock_loop_body, Execute(&mock_runtime)).Times(0);

  // Single error.
  EXPECT_CALL(mock_runtime, EnqueueInternalEvent("error.execution", _));

  EXPECT_FALSE(for_each.Execute(&mock_runtime));
}

// Error declaring the index variable.
TEST(ForEach, IllegalIndexLocationIsError) {
  NiceMock<MockRuntime> mock_runtime;
  auto& mock_datamodel = mock_runtime.GetDefaultMockDatamodel();
  NiceMock<MockExecutableContent> mock_loop_body;

  ON_CALL(mock_datamodel, IsDefined(_)).WillByDefault(Return(false));

  ForEach for_each("[0, 2, 4]", "item", "index", &mock_loop_body);

  // Result owned by 'for_each'.
  EXPECT_CALL(mock_datamodel, EvaluateIterator("[0, 2, 4]"))
      .WillOnce(Return(new MockIterator({"0", "2", "4"})));

  // Legal item.
  EXPECT_CALL(mock_datamodel, Declare("item")).WillOnce(Return(true));

  // Illegal index declaration.
  EXPECT_CALL(mock_datamodel, Declare("index")).WillOnce(Return(false));

  // No assignments.
  EXPECT_CALL(mock_datamodel, AssignExpression(_, _)).Times(0);
  // Loop body never called.
  EXPECT_CALL(mock_loop_body, Execute(&mock_runtime)).Times(0);

  // Single error.
  EXPECT_CALL(mock_runtime, EnqueueInternalEvent("error.execution", _));

  EXPECT_FALSE(for_each.Execute(&mock_runtime));
}

// Loop terminated on body execution error.
TEST(ForEach, LoopTerminatesEarlyOnBodyExecutionError) {
  NiceMock<MockRuntime> mock_runtime;
  auto& mock_datamodel = mock_runtime.GetDefaultMockDatamodel();
  NiceMock<MockExecutableContent> mock_loop_body;

  ON_CALL(mock_datamodel, IsDefined(_)).WillByDefault(Return(false));
  ON_CALL(mock_datamodel, Declare(_)).WillByDefault(Return(true));

  ForEach for_each("[0, 2, 4]", "item", "index", &mock_loop_body);

  // Result owned by 'for_each'.
  EXPECT_CALL(mock_datamodel, EvaluateIterator("[0, 2, 4]"))
      .WillOnce(Return(new MockIterator({"0", "2", "4"})));

  // Body returns an error on the second iteration.
  EXPECT_CALL(mock_loop_body, Execute(&mock_runtime))
      .WillOnce(Return(true))
      .WillOnce(Return(false));

  // Value assigned in right sequence.
  {
    InSequence sequence;
    EXPECT_CALL(mock_datamodel, AssignExpression("item", "0"))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_datamodel, AssignExpression("item", "2"))
        .WillOnce(Return(true));
  }
  // 3rd assignment never called.
  EXPECT_CALL(mock_datamodel, AssignExpression("item", "4")).Times(0);

  // Index assigned in right sequence.
  {
    InSequence sequence;
    EXPECT_CALL(mock_datamodel, AssignExpression("index", "0"))
        .WillOnce(Return(true));
    EXPECT_CALL(mock_datamodel, AssignExpression("index", "1"))
        .WillOnce(Return(true));
  }
  // 3rd assignment never called.
  EXPECT_CALL(mock_datamodel, AssignExpression("index", "2")).Times(0);

  // No error needs to be enqueued by for_each (errors may be enqueued by
  // executables in body).
  EXPECT_CALL(mock_runtime, EnqueueInternalEvent("error.execution", _))
      .Times(0);

  EXPECT_FALSE(for_each.Execute(&mock_runtime));
}

}  // namespace
}  // namespace model
}  // namespace state_chart
