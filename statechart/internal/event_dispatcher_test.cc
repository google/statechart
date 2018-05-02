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

#include "statechart/internal/event_dispatcher.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "statechart/internal/testing/mock_runtime.h"
#include "statechart/internal/testing/mock_state.h"
#include "statechart/state_machine_listener.h"

namespace state_chart {
namespace {

class MockListener : public StateMachineListener {
 public:
  MOCK_METHOD2(OnStateEntered, void(const Runtime*, const model::State*));
  MOCK_METHOD2(OnStateExited, void(const Runtime*, const model::State*));
  MOCK_METHOD2(OnTransitionFollowed,
               void(const Runtime*, const model::Transition*));
  MOCK_METHOD6(OnSendEvent, void(const Runtime*, const string&, const string&,
                                 const string&, const string&, const string&));
};

class EventDispatcherTest : public ::testing::Test {
 protected:
  EventDispatcherTest() { dispatcher_.AddListener(&listener_); }

  MockRuntime runtime_;
  testing::StrictMock<MockListener> listener_;
  EventDispatcher dispatcher_;
};

TEST_F(EventDispatcherTest, OnStateEntered) {
  MockState state("A");
  EXPECT_CALL(listener_,
              OnStateEntered(&runtime_, &state));
  dispatcher_.NotifyStateEntered(&runtime_, &state);
}

TEST_F(EventDispatcherTest, OnStateExited) {
  MockState state("A");
  EXPECT_CALL(listener_,
              OnStateExited(&runtime_, &state));
  dispatcher_.NotifyStateExited(&runtime_, &state);
}

TEST_F(EventDispatcherTest, OnSendEvent) {
  EXPECT_CALL(listener_,
              OnSendEvent(&runtime_, "event", "target", "type", "id", "data"));

  dispatcher_.NotifySendEvent(&runtime_, "event", "target", "type", "id",
                              "data");
}

}  // namespace
}  // namespace state_chart
