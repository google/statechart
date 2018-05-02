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

#include "statechart/internal/runtime_impl.h"

#include "absl/memory/memory.h"
#include "statechart/platform/test_util.h"
#include "statechart/internal/testing/mock_datamodel.h"
#include "statechart/internal/testing/mock_state.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using testing::EqualsProto;
using testing::proto::IgnoringRepeatedFieldOrdering;
using testing::UnorderedElementsAre;

namespace state_chart {
namespace {

class RuntimeImplTest : public ::testing::Test {
 protected:
  RuntimeImplTest()
      : datamodel_(new MockDatamodel()) {
    runtime_ = RuntimeImpl::Create(::absl::WrapUnique(datamodel_));
  }

  MockDatamodel* datamodel_;
  std::unique_ptr<Runtime> runtime_;
};

TEST_F(RuntimeImplTest, IsRunning) {
  ASSERT_FALSE(runtime_->IsRunning());

  runtime_->SetRunning(true);
  ASSERT_TRUE(runtime_->IsRunning());

  runtime_->SetRunning(false);
  ASSERT_FALSE(runtime_->IsRunning());
}

TEST_F(RuntimeImplTest, Datamodel) {
  ASSERT_EQ(datamodel_, runtime_->mutable_datamodel());
  ASSERT_EQ(datamodel_,
            static_cast<RuntimeImpl*>(runtime_.get())->mutable_datamodel());
}

TEST_F(RuntimeImplTest, ActiveStates) {
  EXPECT_THAT(runtime_->GetActiveStates(), UnorderedElementsAre());

  MockState state_A("A");
  runtime_->AddActiveState(&state_A);
  EXPECT_THAT(runtime_->GetActiveStates(), UnorderedElementsAre(&state_A));
  EXPECT_TRUE(runtime_->IsActiveState("A"));
  EXPECT_FALSE(runtime_->IsActiveState("B"));

  MockState state_B("B");
  runtime_->AddActiveState(&state_B);
  EXPECT_THAT(runtime_->GetActiveStates(), UnorderedElementsAre(
      &state_A, &state_B));
  EXPECT_TRUE(runtime_->IsActiveState("A"));
  EXPECT_TRUE(runtime_->IsActiveState("B"));

  runtime_->EraseActiveState(&state_A);
  EXPECT_THAT(runtime_->GetActiveStates(), UnorderedElementsAre(&state_B));

  runtime_->EraseActiveState(&state_B);
  EXPECT_THAT(runtime_->GetActiveStates(), UnorderedElementsAre());
}

TEST_F(RuntimeImplTest, InternalEvents) {
  ASSERT_FALSE(runtime_->HasInternalEvent());

  runtime_->EnqueueInternalEvent("event1", "payload1");
  runtime_->EnqueueInternalEvent("event2", "payload2");
  runtime_->EnqueueInternalEvent("event3", "payload3");

  ASSERT_TRUE(runtime_->HasInternalEvent());
  EXPECT_EQ(std::make_pair(string("event1"), string("payload1")),
            runtime_->DequeueInternalEvent());

  ASSERT_TRUE(runtime_->HasInternalEvent());
  EXPECT_EQ(std::make_pair(string("event2"), string("payload2")),
            runtime_->DequeueInternalEvent());

  ASSERT_TRUE(runtime_->HasInternalEvent());
  EXPECT_EQ(std::make_pair(string("event3"), string("payload3")),
            runtime_->DequeueInternalEvent());

  ASSERT_FALSE(runtime_->HasInternalEvent());
}

TEST_F(RuntimeImplTest, Serialize) {
  // Serialize empty runtime_.
  EXPECT_THAT(runtime_->Serialize(), EqualsProto("running: false"));

  // Populate active states in runtime_.
  MockState state_A("A");
  MockState state_Aa("A.a");
  MockState state_Aa1("A.a.1");
  MockState state_Aa2("A.a.2");
  MockState state_Ab("A.b");
  MockState state_B("B");

  state_Aa.SetParent(&state_A);
  state_Aa1.SetParent(&state_Aa);
  state_Aa2.SetParent(&state_Aa);
  state_Ab.SetParent(&state_A);

  // If we are to remain true to the StateChart standard, such a
  // configuration of active states is not allowed. But since all states are
  // mocks anyway we use this configuration for testing.
  // TODO(srgandhe): Think of better format for serializing Runtime.
  runtime_->AddActiveState(&state_Aa1);
  runtime_->AddActiveState(&state_Aa);
  runtime_->AddActiveState(&state_A);
  runtime_->AddActiveState(&state_Aa2);
  runtime_->AddActiveState(&state_Ab);
  runtime_->AddActiveState(&state_B);

  const char kExpectedRuntime[] = R"( active_state {
                                        id: "A"
                                        active_child {
                                          id: "A.a"
                                          active_child { id: "A.a.1" }
                                          active_child { id: "A.a.2" }
                                        }
                                        active_child { id: "A.b" }
                                      }
                                      active_state { id: "B" }
                                      running: false)";
  EXPECT_THAT(runtime_->Serialize(),
              IgnoringRepeatedFieldOrdering(EqualsProto(kExpectedRuntime)));
}

using RuntimeImplDeathTest = RuntimeImplTest;

TEST_F(RuntimeImplDeathTest, DieIfSerializeOnNonQuiescent) {
  runtime_->EnqueueInternalEvent("E.internal_event", "");
  EXPECT_DEBUG_DEATH(
      runtime_->Serialize(),
      "Trying to serialize a Runtime that has not been allowed to run "
      "to quiescence is not allowed. Returning empty.");
}

}  // namespace
}  // namespace state_chart
