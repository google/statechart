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

#ifndef STATE_CHART_TESTING_MOCK_STATE_MACHINE_H_
#define STATE_CHART_TESTING_MOCK_STATE_MACHINE_H_

#include "statechart/platform/protobuf.h"
#include "statechart/internal/testing/mock_model.h"
#include "statechart/internal/testing/mock_runtime.h"
#include "statechart/state_machine.h"

namespace state_chart {

class MockStateMachine : public StateMachine {
 public:
  MockStateMachine() {
    // Delegate call to SendEvent() with proto payload to the parent class by
    // default.
    ON_CALL(*this, SendEvent(testing::_, testing::An<const proto2::Message*>()))
        .WillByDefault(testing::Invoke(this, &MockStateMachine::RealSendEvent));

    ON_CALL(*this, GetRuntime())
        .WillByDefault(testing::ReturnRef(default_runtime_));
    ON_CALL(*this, GetModel())
        .WillByDefault(testing::ReturnRef(default_model_));
  }

  MockStateMachine(const MockStateMachine&) = delete;
  MockStateMachine& operator=(const MockStateMachine&) = delete;

  MOCK_METHOD0(Start, void());

  MOCK_METHOD2(SendEvent, void(const string&, const string&));
  MOCK_METHOD2(SendEvent, void(const string&, const proto2::Message*));

  MOCK_CONST_METHOD0(GetRuntime, const Runtime&());
  MOCK_CONST_METHOD0(GetModel, const Model&());

  MOCK_METHOD1(AddListener, void(StateMachineListener* listener));

  void RealSendEvent(const string& event, const proto2::Message* message) {
    StateMachine::SendEvent(event, message);
  }

  MockRuntime& GetDefaultMockRuntime() { return default_runtime_; }

 private:
  MockRuntime default_runtime_;
  MockModel default_model_;
};

}  // namespace state_chart

#endif  // STATE_CHART_TESTING_MOCK_STATE_MACHINE_H_
