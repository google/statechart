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

#ifndef STATE_CHART_INTERNAL_TESTING_MOCK_RUNTIME_H_
#define STATE_CHART_INTERNAL_TESTING_MOCK_RUNTIME_H_

#include <set>
#include <string>
#include <utility>

#include <gmock/gmock.h>

#include "statechart/internal/datamodel.h"
#include "statechart/internal/event_dispatcher.h"
#include "statechart/internal/runtime.h"
#include "statechart/internal/testing/mock_datamodel.h"
#include "statechart/internal/testing/mock_event_dispatcher.h"
#include "statechart/proto/state_machine_context.pb.h"

namespace state_chart {
namespace model {
class State;
}  // namespace model
}  // namespace state_chart

namespace state_chart {

class MockRuntime : public Runtime {
 public:
  MockRuntime() : is_running_(false) {
    using ::testing::Const;
    using ::testing::Return;

    ON_CALL(*this, mutable_datamodel())
        .WillByDefault(Return(&default_datamodel_));
    ON_CALL(*this, HasInternalEvent()).WillByDefault(Return(false));
    ON_CALL(Const(*this), datamodel())
        .WillByDefault(ReturnRef(default_datamodel_));
    ON_CALL(*this, GetEventDispatcher())
        .WillByDefault(Return(&default_event_dispatcher_));
  }
  MockRuntime(const MockRuntime&) = delete;
  MockRuntime& operator=(const MockRuntime&) = delete;
  ~MockRuntime() override = default;

  MOCK_CONST_METHOD0(GetActiveStates, std::set<const model::State*>());
  MOCK_CONST_METHOD1(IsActiveState, bool(const string&));
  MOCK_METHOD1(AddActiveState, void(const model::State* state));
  MOCK_METHOD1(EraseActiveState, void(const model::State* state));

  bool IsRunning() const override { return is_running_; }
  void SetRunning(bool is_running) override { is_running_ = is_running; }

  MOCK_CONST_METHOD0(HasInternalEvent, bool());
  MOCK_METHOD0(DequeueInternalEvent, std::pair<string, string>());
  MOCK_METHOD2(EnqueueInternalEvent,
               void(const string& event, const string& payload));

  MOCK_METHOD0(mutable_datamodel, Datamodel*());
  MOCK_CONST_METHOD0(datamodel, const Datamodel&());

  MockDatamodel& GetDefaultMockDatamodel() { return default_datamodel_; }

  MOCK_METHOD0(GetEventDispatcher, EventDispatcher*());

  MockEventDispatcher& GetDefaultMockEventDispatcher() {
    return default_event_dispatcher_;
  }

  MOCK_METHOD0(Clear, void());

  string DebugString() const override { return "MockRuntime"; }

  MOCK_CONST_METHOD0(Serialize, StateMachineContext::Runtime());

 private:
  bool is_running_;

  testing::NiceMock<MockDatamodel> default_datamodel_;
  testing::NiceMock<MockEventDispatcher> default_event_dispatcher_;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_TESTING_MOCK_RUNTIME_H_
