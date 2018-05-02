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

#include "statechart/internal/model/send.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "statechart/internal/testing/mock_datamodel.h"
#include "statechart/internal/testing/mock_event_dispatcher.h"
#include "statechart/internal/testing/mock_runtime.h"

namespace state_chart {
namespace model {
namespace {

using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Pair;
using ::testing::Return;

ACTION(ReturnEvaluationDefaultResult) {
  *arg1 = arg0 + "_result";
  return true;
}

class SendTest : public testing::Test {
 public:
  SendTest() {
    ON_CALL(runtime_.GetDefaultMockDatamodel(), EvaluateStringExpression(_, _))
        .WillByDefault(ReturnEvaluationDefaultResult());
    ON_CALL(runtime_.GetDefaultMockDatamodel(), EvaluateExpression(_, _))
        .WillByDefault(ReturnEvaluationDefaultResult());
    ON_CALL(runtime_.GetDefaultMockDatamodel(), EncodeParameters(_))
        .WillByDefault(Return(""));
  }

 protected:
  MockRuntime runtime_;
};

// Test valid send attributes with possibly unspecified attributes.
TEST_F(SendTest, ValidSendAttributes) {
  Send send_A(Expr("event_A"), "target_A", "id_A", Expr("type_A"));
  EXPECT_CALL(runtime_.GetDefaultMockEventDispatcher(),
              NotifySendEvent(&runtime_, "event_A_result", "target_A",
                              "type_A_result", "id_A", ""));
  EXPECT_TRUE(send_A.Execute(&runtime_));

  Send send_B("", "", "", "");
  EXPECT_CALL(runtime_.GetDefaultMockEventDispatcher(),
              NotifySendEvent(&runtime_, "", "", "", "", ""));
  EXPECT_TRUE(send_B.Execute(&runtime_));

  Send send_C("", Expr("target_C"), "id_C", "");
  EXPECT_CALL(
      runtime_.GetDefaultMockEventDispatcher(),
      NotifySendEvent(&runtime_, "", "target_C_result", "", "id_C", ""));
  EXPECT_TRUE(send_C.Execute(&runtime_));
}

// Test that internal event is enqueued when there is an error in evaluating
// some send attribute.
TEST_F(SendTest, ErrorInSendAttributes) {
  Send send_A("event_A", "target_A", Expr("id_A"), "type_A");

  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateStringExpression(_, _))
      .WillRepeatedly(ReturnEvaluationDefaultResult());
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateStringExpression("id_A", _)).WillOnce(Return(false));
  EXPECT_CALL(runtime_, EnqueueInternalEvent("error.execution", _));

  EXPECT_CALL(runtime_.GetDefaultMockEventDispatcher(),
              NotifySendEvent(&runtime_, _, _, _, _, _)).Times(0);

  EXPECT_FALSE(send_A.Execute(&runtime_));
}

// Test send with parameters.
TEST_F(SendTest, ValidSendParameters) {
  Send send_A("", "", "", "");
  EXPECT_TRUE(send_A.AddParamByExpression("param1", "expr1"));
  EXPECT_TRUE(send_A.AddParamByExpression("param2", "expr2"));
  EXPECT_TRUE(send_A.AddParamByExpression("param3", "expr3"));

  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EncodeParameters(ElementsAre(Pair("param1", "expr1_result"),
                                           Pair("param2", "expr2_result"),
                                           Pair("param3", "expr3_result"))))
      .WillOnce(Return("result"));

  EXPECT_CALL(runtime_.GetDefaultMockEventDispatcher(),
              NotifySendEvent(&runtime_, "", "", "", "", "result"));

  EXPECT_TRUE(send_A.Execute(&runtime_));
}

// Test that internal event is enqueued when there is an error in evaluating
// some send parameter.
TEST_F(SendTest, ErrorInSendParameters) {
  Send send_A("", "", "", "");
  EXPECT_TRUE(send_A.AddParamByExpression("param1", "expr1"));
  EXPECT_TRUE(send_A.AddParamByExpression("param2", "expr2"));
  EXPECT_TRUE(send_A.AddParamByExpression("param3", "expr3"));

  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(), EvaluateExpression(_, _))
      .WillRepeatedly(ReturnEvaluationDefaultResult());
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateExpression("expr2", _)).WillOnce(Return(false));
  EXPECT_CALL(runtime_, EnqueueInternalEvent("error.execution", _));

  // Note that the error parameter is ignored, while the rest are processed.
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EncodeParameters(ElementsAre(Pair("param1", "expr1_result"),
                                           Pair("param3", "expr3_result"))))
      .WillOnce(Return("result"));
  EXPECT_CALL(runtime_.GetDefaultMockEventDispatcher(),
              NotifySendEvent(&runtime_, _, _, _, _, "result"));

  EXPECT_FALSE(send_A.Execute(&runtime_));
}

}  // namespace
}  // namespace model
}  // namespace state_chart
