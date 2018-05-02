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

#include "statechart/internal/model/log.h"

#include "statechart/platform/test_util.h"
#include "statechart/internal/testing/mock_datamodel.h"
#include "statechart/internal/testing/mock_runtime.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace state_chart {
namespace model {
namespace {

using ::testing::_;
using ::testing::kDoNotCaptureLogsYet;
using ::testing::NotNull;
using ::testing::ScopedMockLog;

const char kLogCCPath[] = "statechart/internal/model/log.cc";

TEST(LogTest, Error) {
  MockRuntime runtime;
  EXPECT_CALL(runtime.GetDefaultMockDatamodel(),
              EvaluateStringExpression("expression", NotNull()))
      .WillOnce(ReturnEvaluationError());
  EXPECT_CALL(runtime, EnqueueInternalEvent("error.execution", _));

  ScopedMockLog mock_log(kDoNotCaptureLogsYet);
  EXPECT_CALL(mock_log, Log(base_logging::INFO, kLogCCPath, _)).Times(0);

  Log log("", "expression");
  mock_log.StartCapturingLogs();
  EXPECT_FALSE(log.Execute(&runtime));
}

TEST(LogTest, NoLabel) {
  MockRuntime runtime;
  EXPECT_CALL(runtime.GetDefaultMockDatamodel(),
              EvaluateStringExpression("expression", NotNull()))
      .WillOnce(ReturnEvaluationResult("log line"));

  ScopedMockLog mock_log(kDoNotCaptureLogsYet);
  EXPECT_CALL(mock_log, Log(base_logging::INFO, kLogCCPath, "log line"));

  Log log("", "expression");
  mock_log.StartCapturingLogs();
  EXPECT_TRUE(log.Execute(&runtime));
}

TEST(LogTest, Label) {
  MockRuntime runtime;
  EXPECT_CALL(runtime.GetDefaultMockDatamodel(),
              EvaluateStringExpression("expression", NotNull()))
      .WillOnce(ReturnEvaluationResult("log line"));

  ScopedMockLog mock_log(kDoNotCaptureLogsYet);
  EXPECT_CALL(mock_log, Log(base_logging::INFO, kLogCCPath, "label: log line"));

  Log log("label", "expression");
  mock_log.StartCapturingLogs();
  log.Execute(&runtime);
}

}  // namespace
}  // namespace model
}  // namespace state_chart
