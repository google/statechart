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

#include "statechart/internal/model/str_or_expr.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "statechart/internal/testing/mock_datamodel.h"

namespace state_chart {
namespace model {
namespace {

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgPointee;

TEST(StrOrExprTest, TestStrOrExpr) {
  MockDatamodel datamodel;
  string result;

  LOG(INFO) << "Test that string is created and it is not evaluated.";
  StrOrExpr str1("str1");
  EXPECT_CALL(datamodel, EvaluateStringExpression(_, _)).Times(0);
  EXPECT_TRUE(str1.Evaluate(&datamodel, &result));
  EXPECT_EQ("str1", result);

  LOG(INFO) << "Test that an expression is created and evaluated.";
  StrOrExpr expr1(Expr("expr1"));
  EXPECT_CALL(datamodel, EvaluateStringExpression("expr1", _))
      .WillOnce(DoAll(SetArgPointee<1>("result1"), Return(true)));
  EXPECT_TRUE(expr1.Evaluate(&datamodel, &result));
  EXPECT_EQ("result1", result);

  LOG(INFO) << "Test error in expression evaluation.";
  StrOrExpr expr2(Expr("malformed"));
  EXPECT_CALL(datamodel, EvaluateStringExpression("malformed", _))
      .WillOnce(Return(false));
  EXPECT_FALSE(expr2.Evaluate(&datamodel, &result));
}

}  // namespace
}  // namespace model
}  // namespace state_chart
