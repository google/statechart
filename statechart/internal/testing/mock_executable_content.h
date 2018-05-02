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

#ifndef STATE_CHART_INTERNAL_TESTING_MOCK_EXECUTABLE_CONTENT_H_
#define STATE_CHART_INTERNAL_TESTING_MOCK_EXECUTABLE_CONTENT_H_

#include "statechart/internal/model/executable_content.h"

#include <gmock/gmock.h>

namespace state_chart {

class Runtime;

class MockExecutableContent : public model::ExecutableContent {
 public:
  MockExecutableContent() {
    using ::testing::_;
    using ::testing::Return;
    ON_CALL(*this, Execute(_)).WillByDefault(Return(true));
  }
  MockExecutableContent(const MockExecutableContent&) = delete;
  MockExecutableContent& operator=(const MockExecutableContent&) = delete;
  ~MockExecutableContent() override = default;

  MOCK_CONST_METHOD1(Execute, bool(Runtime*));
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_TESTING_MOCK_EXECUTABLE_CONTENT_H_
