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

#include "statechart/state_machine.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "statechart/internal/testing/mock_datamodel.h"
#include "statechart/internal/testing/mock_runtime.h"
#include "statechart/platform/test_util.h"
#include "statechart/proto/state_machine_context.pb.h"
#include "statechart/testing/mock_state_machine.h"
#include "statechart/testing/state_machine_test.pb.h"


using proto2::contrib::parse_proto::ParseTextOrDie;
using testing::AtLeast;
using testing::EqualsProtobuf;
using testing::NotNull;
using testing::Return;

namespace state_chart {
namespace {

TEST(StateMachineTest, SendMessage_WithProto) {
  MockStateMachine sm;

  // Without payload.
  EXPECT_CALL(sm, SendEvent("no_payload", ""));
  sm.SendEvent("no_payload", nullptr);

  // With payload.
  StateMachineTestMessage message;
  message.set_value(10);

  EXPECT_CALL(sm, SendEvent("with_payload", "{\n  \"value\": 10\n}\n"));
  sm.SendEvent("with_payload", &message);
}

TEST(StateMachineTest, ExtractMessageFromDatamodel) {
  MockStateMachine sm;

  MockDatamodel& datamodel =
      sm.GetDefaultMockRuntime().GetDefaultMockDatamodel();

  // Test error cases first: location does not exists error, then JSON
  // conversion error.
  EXPECT_CALL(datamodel, IsDefined("error_location")).WillOnce(Return(false));

  StateMachineTestMessage message;
  ASSERT_FALSE(sm.ExtractMessageFromDatamodel("error_location", &message));

  // JSON conversion error.
  EXPECT_CALL(datamodel, IsDefined("malformed_location"))
      .WillOnce(Return(true));
  EXPECT_CALL(datamodel, EvaluateExpression("malformed_location", NotNull()))
      .WillOnce(ReturnEvaluationResult("{\"value\": \"string_in_int_field\"}"));

  ASSERT_FALSE(sm.ExtractMessageFromDatamodel("malformed_location", &message));

  // Success case.
  EXPECT_CALL(datamodel, IsDefined("json_location")).WillOnce(Return(true));
  EXPECT_CALL(datamodel, EvaluateExpression("json_location", NotNull()))
      .WillOnce(ReturnEvaluationResult("{\"value\": 5}"));

  ASSERT_TRUE(sm.ExtractMessageFromDatamodel("json_location", &message));

  EXPECT_EQ(5, message.value());
}

TEST(StateMachineTest, ExtractMessageFromDatamodel_Extensions) {
  MockStateMachine sm;

  MockDatamodel& datamodel =
      sm.GetDefaultMockRuntime().GetDefaultMockDatamodel();

  EXPECT_CALL(datamodel, IsDefined("json_location")).WillOnce(Return(true));
  EXPECT_CALL(datamodel, EvaluateExpression("json_location", NotNull()))
      .WillOnce(ReturnEvaluationResult("{\"foo\": { \"ext_value\": 10 }}"));

  StateMachineTestMessage message;
  ASSERT_TRUE(sm.ExtractMessageFromDatamodel("json_location", &message));

  EXPECT_TRUE(message.HasExtension(StateMachineTestExtension::ext));
  EXPECT_EQ(10, message.GetExtension(
      StateMachineTestExtension::ext).ext_value());
}

TEST(StateMachineTest, SerializeToContext) {
  MockStateMachine sm;

  MockRuntime& runtime = sm.GetDefaultMockRuntime();
  MockDatamodel& datamodel = runtime.GetDefaultMockDatamodel();

  EXPECT_CALL(sm, GetRuntime()).Times(AtLeast(1));
  EXPECT_CALL(runtime, HasInternalEvent()).WillRepeatedly(Return(false));

  const StateMachineContext kExpectedStateMachineContext =
      ParseTextOrDie<StateMachineContext>(R"(
          runtime {
            active_state {
              id: "A"
              active_child { id : "A.a" }
              active_child { id : "A.b" }
            }
            active_state { id: "B" }
            running: true
          }
          datamodel: "something serialized" )");

  EXPECT_CALL(runtime, Serialize())
      .WillOnce(Return(kExpectedStateMachineContext.runtime()));
  EXPECT_CALL(datamodel, SerializeAsString())
      .WillOnce(Return(kExpectedStateMachineContext.datamodel()));

  StateMachineContext state_machine_context;
  EXPECT_TRUE(sm.SerializeToContext(&state_machine_context));
  EXPECT_THAT(state_machine_context, EqualsProtobuf(kExpectedStateMachineContext));
}

}  // namespace
}  // namespace state_chart
