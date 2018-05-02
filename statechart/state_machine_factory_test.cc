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

#include "statechart/state_machine_factory.h"

#include <set>

#include "statechart/internal/function_dispatcher.h"
#include "statechart/internal/model.h"
#include "statechart/internal/model/model.h"
#include "statechart/internal/runtime.h"
#include "statechart/internal/testing/mock_function_dispatcher.h"
#include "statechart/internal/testing/state_chart_builder.h"
#include "statechart/platform/test_util.h"
#include "statechart/proto/state_chart.pb.h"
#include "statechart/state_machine.h"

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using proto2::contrib::parse_proto::ParseTextOrDie;
using testing::NiceMock;
using testing::UnorderedElementsAre;

namespace state_chart {
namespace {

// Used to expose protected members for testing.
class TestStateMachineFactory : public StateMachineFactory {
 public:
  TestStateMachineFactory() {}
  using StateMachineFactory::AddModelFromProto;

  FunctionDispatcher* mutable_function_dispatcher() { return &dispatcher_; }

 private:
  // A default implementation for function dispatcher.
  NiceMock<MockFunctionDispatcher> dispatcher_;
};

// Test the empty factory case.
TEST(TestStateMachineFactory, NoModelsTest) {
  TestStateMachineFactory factory;

  EXPECT_FALSE(factory.HasModel("imaginary_model"));
  EXPECT_DEBUG_DEATH(
      {
        EXPECT_EQ(nullptr, factory.CreateStateMachine(
                               "imaginary_model",
                               factory.mutable_function_dispatcher()));
      },
      "Returning nullptr; condition \\(model == nullptr || function_dispatcher "
      "== nullptr\\) is true.");
}

// Test that state machines can be created from added models.
TEST(TestStateMachineFactory, CanCreateAfterAdding) {
  TestStateMachineFactory factory;

  config::StateChart state_chart1;
  config::StateChartBuilder(&state_chart1, "model1").AddState("model1.state");
  factory.AddModelFromProto(state_chart1);

  config::StateChart state_chart2;
  config::StateChartBuilder(&state_chart2, "model2").AddState("model2.state");
  factory.AddModelFromProto(state_chart2);

  std::unique_ptr<StateMachine> state_machine;

  state_machine = factory.CreateStateMachine(
      "model2", factory.mutable_function_dispatcher());
  ASSERT_NE(nullptr, state_machine.get());
  EXPECT_EQ("model2", state_machine->GetModelName());

  state_machine = factory.CreateStateMachine(
      "model1", factory.mutable_function_dispatcher());
  ASSERT_NE(nullptr, state_machine.get());
  EXPECT_EQ("model1", state_machine->GetModelName());

  EXPECT_TRUE(factory.HasModel("model1"));
  EXPECT_TRUE(factory.HasModel("model2"));
  EXPECT_FALSE(factory.HasModel("imaginary_model"));

  EXPECT_DEBUG_DEATH(
      {
        EXPECT_EQ(nullptr, factory.CreateStateMachine(
                               "imaginary_model",
                               factory.mutable_function_dispatcher()));
      },
      "Returning nullptr; condition \\(model == nullptr || function_dispatcher "
      "== nullptr\\) is true.");
}

TEST(TestStateMachineFactory, CreateStateMachineFromStateMachineContext) {
  TestStateMachineFactory factory;

  config::StateChart state_chart1 = ParseTextOrDie<config::StateChart>(R"(
      name: "model1"
      state {
        parallel {
          id: "top"
          state {
            parallel {
              id: "A"
              state { state { id: "A.a" } }
              state { state { id: "A.b" } }
            }
          }
          state { state { id: "B" } }
        }
      }
  )");
  factory.AddModelFromProto(state_chart1);

  const StateMachineContext state_machine_context =
      ParseTextOrDie<StateMachineContext>(R"(
            runtime {
              active_state {
                id: "top"
                active_child {
                  id: "A"
                  active_child { id : "A.a" }
                  active_child { id : "A.b" }
                }
                active_child { id: "B" }
              }
              running: true
            }
            datamodel: "null\n" )");

  std::unique_ptr<StateMachine> state_machine = factory.CreateStateMachine(
      "model1", state_machine_context, factory.mutable_function_dispatcher());
  EXPECT_NE(nullptr, state_machine);
  EXPECT_TRUE(state_machine->GetRuntime().IsRunning());
  EXPECT_EQ("model1", state_machine->GetModelName());
  std::set<string> active_state_ids;
  for (const model::State* active_state :
       state_machine->GetRuntime().GetActiveStates()) {
    active_state_ids.insert(active_state->id());
  }
  EXPECT_THAT(active_state_ids,
              UnorderedElementsAre("top", "A", "B", "A.a", "A.b"));
}

TEST(StateMachineFactoryTest, CreateFromProtos) {
  std::vector<config::StateChart> state_charts(2);
  config::StateChartBuilder(&state_charts[0], "model1")
      .AddState("model1.state");
  config::StateChartBuilder(&state_charts[1], "model2")
      .AddState("model2.state");

  auto state_machine_factory =
      StateMachineFactory::CreateFromProtos(state_charts);
  EXPECT_NE(nullptr, state_machine_factory);
  EXPECT_TRUE(state_machine_factory->HasModel("model1"));
  EXPECT_TRUE(state_machine_factory->HasModel("model2"));
  EXPECT_FALSE(state_machine_factory->HasModel("model3"));
}

}  // namespace
}  // namespace state_chart
