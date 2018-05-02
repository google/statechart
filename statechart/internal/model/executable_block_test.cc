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

#include "statechart/internal/model/executable_block.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

#include "statechart/internal/testing/mock_executable_content.h"
#include "statechart/internal/testing/mock_runtime.h"

namespace state_chart {
namespace model {
namespace {

using ::testing::Return;

TEST(ExecutableBlock, ExecutableBlockOrderingTest) {
  MockExecutableContent exec1, exec2, exec3, exec4;
  MockRuntime runtime;

  testing::InSequence sequence;
  EXPECT_CALL(exec1, Execute(&runtime)).WillOnce(Return(true));
  EXPECT_CALL(exec2, Execute(&runtime)).WillOnce(Return(true));
  EXPECT_CALL(exec3, Execute(&runtime)).WillOnce(Return(true));
  EXPECT_CALL(exec4, Execute(&runtime)).WillOnce(Return(true));

  ExecutableBlock block({&exec1, &exec2, &exec3, &exec4});
  EXPECT_TRUE(block.Execute(&runtime));
}

TEST(ExecutableBlock, ExecutableBlockOrderingTestWithError) {
  MockExecutableContent exec1, exec2, exec3, exec4;
  MockRuntime runtime;

  testing::InSequence sequence;
  EXPECT_CALL(exec1, Execute(&runtime)).WillOnce(Return(true));
  EXPECT_CALL(exec2, Execute(&runtime)).WillOnce(Return(true));
  EXPECT_CALL(exec3, Execute(&runtime)).WillOnce(Return(false));
  EXPECT_CALL(exec4, Execute(&runtime)).Times(0);

  ExecutableBlock block({&exec1, &exec2, &exec3, &exec4});
  EXPECT_FALSE(block.Execute(&runtime));
}

}  // namespace
}  // namespace model
}  // namespace state_chart
