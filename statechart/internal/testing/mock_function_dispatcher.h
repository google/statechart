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

#ifndef STATE_CHART_INTERNAL_TESTING_MOCK_FUNCTION_DISPATCHER_H_
#define STATE_CHART_INTERNAL_TESTING_MOCK_FUNCTION_DISPATCHER_H_

#include <gmock/gmock.h>

#include "statechart/platform/types.h"
#include "statechart/internal/function_dispatcher.h"

namespace Json {
class Value;
}  // namespace Json

namespace state_chart {

class MockFunctionDispatcher : public FunctionDispatcher {
 public:
  MockFunctionDispatcher() {
    using testing::_;
    using testing::Const;
    using testing::Return;
    ON_CALL(Const(*this), HasFunction(_)).WillByDefault(Return(false));
    ON_CALL(*this, Execute(_, _, _)).WillByDefault(Return(false));
  }
  MockFunctionDispatcher(const MockFunctionDispatcher&) = delete;
  MockFunctionDispatcher& operator=(const MockFunctionDispatcher&) = delete;
  ~MockFunctionDispatcher() override = default;

  MOCK_CONST_METHOD1(HasFunction, bool(const string&));
  MOCK_METHOD3(Execute,
               bool(const string&, const std::vector<const Json::Value*>&,
                    Json::Value*));
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_TESTING_MOCK_FUNCTION_DISPATCHER_H_
