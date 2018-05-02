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

#include "statechart/internal/executor.h"

#include <set>
#include <utility>
#include <vector>

#include <glog/logging.h>

#include "statechart/platform/protobuf.h"
#include "statechart/internal/testing/delegating_mock_executor.h"
#include "statechart/internal/testing/mock_datamodel.h"
#include "statechart/internal/testing/mock_event_dispatcher.h"
#include "statechart/internal/testing/mock_executable_content.h"
#include "statechart/internal/testing/mock_model.h"
#include "statechart/internal/testing/mock_runtime.h"
#include "statechart/internal/testing/mock_state.h"
#include "statechart/internal/testing/mock_transition.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::_;
using testing::InSequence;
using testing::Mock;
using testing::NiceMock;
using testing::Pointee;
using testing::Return;
using testing::SetArgPointee;
using testing::UnorderedElementsAre;

namespace state_chart {
namespace {

class ExecutorTest : public ::testing::Test {
 protected:
  // Create the top level initial transition and setup the default call for it.
  void CreateInitialTransition(const model::State* target) {
    model_.CreateInitialTransition(target);
  }

  void Start() { executor_.Start(&model_, &runtime_); }

  void SendEvent(const string& event, const string& payload) {
    executor_.SendEvent(&model_, &runtime_, event, payload);
  }

  void ExecuteUntilStable() {
    executor_.ExecuteUntilStable(&model_, &runtime_);
  }

  void EnterStates(const std::vector<const model::Transition*>& transitions) {
    executor_.EnterStates(&model_, &runtime_, transitions);
  }

  void MicroStep(const std::vector<const model::Transition*>& transitions) {
    executor_.MicroStep(&model_, &runtime_, transitions);
  }

  void ExitStates(const std::vector<const model::Transition*>& transitions) {
    executor_.ExitStates(&model_, &runtime_, transitions);
  }

  NiceMock<MockRuntime> runtime_;
  NiceMock<MockModel> model_;
  NiceMock<DelegatingMockExecutor> executor_;
};

// No transitions for internal execution.
TEST_F(ExecutorTest, NoTransitionsForInternal) {
  MockState state("S");
  CreateInitialTransition(&state);

  // Expect that the system variables are set up.
  EXPECT_CALL(runtime_, Clear());
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(), Declare("_name"))
      .WillOnce(Return(true));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(), Declare("_sessionid"))
      .WillOnce(Return(true));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(), Declare("_event"))
      .WillOnce(Return(true));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(), AssignString("_name", _))
      .WillOnce(Return(true));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(), AssignString("_sessionid", _))
      .WillOnce(Return(true));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(), AssignExpression("_event", _))
      .WillOnce(Return(true));

  // Top-level states required for datamodel initialization.
  EXPECT_CALL(model_, GetTopLevelStates()).WillOnce(Return(StateVec{&state}));
  EXPECT_CALL(model_, GetEventlessTransitions(&runtime_))
      .WillOnce(Return(Transitions()));
  // Initial state should be added.
  EXPECT_CALL(runtime_, AddActiveState(&state));

  Start();
  ASSERT_TRUE(runtime_.IsRunning());
}

// Start() executes until stable.
TEST_F(ExecutorTest, StartExecuteTillStable) {
  MockState state_A("A");
  MockState state_B("B");
  MockState state_C("C");

  CreateInitialTransition(&state_A);
  MockTransition transition_AB(&state_A, &state_B);
  MockTransition transition_BC(&state_B, &state_C);

  EXPECT_CALL(model_, GetTopLevelStates())
      .WillOnce(Return(StateVec{&state_A, &state_B, &state_C}));

  EXPECT_CALL(model_, GetEventlessTransitions(&runtime_))
      .WillOnce(Return(Transitions{&transition_AB}))
      .WillOnce(Return(Transitions{&transition_BC}))
      .WillOnce(Return(Transitions()));  // Last call always empty.

  InSequence sequence;
  EXPECT_CALL(runtime_, AddActiveState(&state_A));
  EXPECT_CALL(runtime_, EraseActiveState(&state_A));
  EXPECT_CALL(runtime_, AddActiveState(&state_B));
  EXPECT_CALL(runtime_, EraseActiveState(&state_B));
  EXPECT_CALL(runtime_, AddActiveState(&state_C));

  Start();
  ASSERT_TRUE(runtime_.IsRunning());
}

// No transitions matched the external event.
TEST_F(ExecutorTest, NoTransitionsMatchedExternal) {
  EXPECT_CALL(model_, GetTransitionsForEvent(&runtime_, "E"))
      .WillOnce(Return(Transitions()));

  // Required by SendEvent().
  runtime_.SetRunning(true);
  SendEvent("E", "");
}

// A single transition matches.
TEST_F(ExecutorTest, SimpleAtoBMatch) {
  MockState state_A("A");
  MockState state_B("B");

  MockTransition transition(&state_A, &state_B);

  // Required by SendEvent().
  runtime_.SetRunning(true);
  InSequence sequence;

  EXPECT_CALL(model_, GetTransitionsForEvent(&runtime_, "E"))
      .WillOnce(Return(Transitions{&transition}));

  EXPECT_CALL(runtime_, EraseActiveState(&state_A));
  EXPECT_CALL(runtime_, AddActiveState(&state_B));

  SendEvent("E", "");
}

// An event that triggers two separate transitions simultaneously.
TEST_F(ExecutorTest, SimpleDoubleTransition) {
  // Model:
  //   A -E-> B
  //   C -E-> D
  // Event: E
  // Result: B, D
  // Assume this is the document order.
  MockState state_A("A");
  MockState state_B("B");
  MockState state_C("C");
  MockState state_D("D");

  MockTransition transition_AB(&state_A, &state_B);
  MockTransition transition_CD(&state_C, &state_D);

  // Required by SendEvent().
  runtime_.SetRunning(true);

  InSequence sequence;

  EXPECT_CALL(model_, GetTransitionsForEvent(&runtime_, "E"))
      .WillOnce(Return(Transitions{&transition_AB, &transition_CD}));

  // Note that all source states are exited before all target states are
  // entered.
  EXPECT_CALL(runtime_, EraseActiveState(&state_A));
  EXPECT_CALL(runtime_, EraseActiveState(&state_C));

  EXPECT_CALL(runtime_, AddActiveState(&state_B));
  EXPECT_CALL(runtime_, AddActiveState(&state_D));

  SendEvent("E", "");
}

// A single transition match before executing another transition until stable.
TEST_F(ExecutorTest, ExternalAtoBInternalBtoC) {
  MockState state_A("A");
  MockState state_B("B");
  MockState state_C("C");

  MockTransition transition_AB(&state_A, &state_B);
  MockTransition transition_BC(&state_B, &state_C);

  // Required by SendEvent().
  runtime_.SetRunning(true);

  InSequence sequence;

  EXPECT_CALL(executor_, AssignEventData(&runtime_, "E", ""));
  EXPECT_CALL(model_, GetTransitionsForEvent(&runtime_, "E"))
      .WillOnce(Return(Transitions{&transition_AB}));

  EXPECT_CALL(runtime_, EraseActiveState(&state_A));
  EXPECT_CALL(runtime_, AddActiveState(&state_B));

  EXPECT_CALL(model_, GetEventlessTransitions(&runtime_))
      .WillOnce(Return(Transitions{&transition_BC}));

  EXPECT_CALL(runtime_, EraseActiveState(&state_B));
  EXPECT_CALL(runtime_, AddActiveState(&state_C));

  EXPECT_CALL(model_, GetEventlessTransitions(&runtime_))
      .WillOnce(Return(Transitions()));

  SendEvent("E", "");
}

// Test Start() and SendEvent() together.
TEST_F(ExecutorTest, InternalAtoBExternalBtoALoop) {
  // Model: A -> B -E-> A
  // Event: E
  // Resultant state: B
  MockState state_A("A");
  MockState state_B("B");

  CreateInitialTransition(&state_A);
  MockTransition transition_AB(&state_A, &state_B);
  MockTransition transition_BA(&state_B, &state_A);

  EXPECT_CALL(model_, GetTopLevelStates())
      .WillOnce(Return(StateVec{&state_A, &state_B}));

  InSequence sequence;
  // Initial state.
  EXPECT_CALL(runtime_, AddActiveState(&state_A));

  EXPECT_CALL(model_, GetEventlessTransitions(&runtime_))
      .WillOnce(Return(Transitions{&transition_AB}));

  EXPECT_CALL(runtime_, EraseActiveState(&state_A));
  EXPECT_CALL(runtime_, AddActiveState(&state_B));

  // This terminates loop execution in Start().
  EXPECT_CALL(model_, GetEventlessTransitions(&runtime_))
      .WillOnce(Return(Transitions()));

  EXPECT_CALL(executor_, AssignEventData(&runtime_, "E", ""));
  EXPECT_CALL(model_, GetTransitionsForEvent(&runtime_, "E"))
      .WillOnce(Return(Transitions{&transition_BA}));

  EXPECT_CALL(runtime_, EraseActiveState(&state_B));
  EXPECT_CALL(runtime_, AddActiveState(&state_A));

  EXPECT_CALL(model_, GetEventlessTransitions(&runtime_))
      .WillOnce(Return(Transitions{&transition_AB}));

  EXPECT_CALL(runtime_, EraseActiveState(&state_A));
  EXPECT_CALL(runtime_, AddActiveState(&state_B));

  // This terminates loop execution in SendEvent().
  EXPECT_CALL(model_, GetEventlessTransitions(&runtime_))
      .WillOnce(Return(Transitions()));

  Start();
  SendEvent("E", "");
}

// Test internal events in ExecuteUntilStable().
TEST_F(ExecutorTest, InternalEventAtoBtoA) {
  // Model: A -I-> B -I-> A
  // Events: I1, I2
  // Resultant state: A
  MockState state_A("A");
  MockState state_B("B");

  MockTransition transition_AB(&state_A, &state_B);
  MockTransition transition_BA(&state_B, &state_A);

  runtime_.SetRunning(true);

  // This controls loop termination in ExecuteUntilStable().
  EXPECT_CALL(runtime_, HasInternalEvent())
      .WillOnce(Return(true))
      .WillOnce(Return(true))
      .WillOnce(Return(false));

  // Ensure that dequeuing happens in order.
  {
    InSequence event_sequence;

    EXPECT_CALL(runtime_, DequeueInternalEvent())
        .WillOnce(Return(std::make_pair("I1", "")));
    EXPECT_CALL(executor_, AssignEventData(&runtime_, "I1", ""));
    EXPECT_CALL(model_, GetTransitionsForEvent(&runtime_, "I1"))
        .WillOnce(Return(Transitions{&transition_AB}));

    EXPECT_CALL(runtime_, DequeueInternalEvent())
        .WillOnce(Return(std::make_pair("I2", "")));
    EXPECT_CALL(executor_, AssignEventData(&runtime_, "I2", ""));
    EXPECT_CALL(model_, GetTransitionsForEvent(&runtime_, "I2"))
        .WillOnce(Return(Transitions{&transition_BA}));
  }
  // Ensure that all microsteps execute in the correct order.
  {
    InSequence microstep_sequence;

    EXPECT_CALL(runtime_, EraseActiveState(&state_A));
    EXPECT_CALL(runtime_, AddActiveState(&state_B));
    EXPECT_CALL(runtime_, EraseActiveState(&state_B));
    EXPECT_CALL(runtime_, AddActiveState(&state_A));
  }
  ExecuteUntilStable();
}

// Test internal events interleaved with eventless execution in
// ExecuteUntilStable().
TEST_F(ExecutorTest, InternalAtoBInternalEventBtoA) {
  // Model: A -> B -I-> A
  // Event: I
  // Resultant state: B
  MockState state_A("A");
  MockState state_B("B");

  MockTransition transition_AB(&state_A, &state_B);
  MockTransition transition_BA(&state_B, &state_A);

  runtime_.SetRunning(true);

  // This controls loop termination in ExecuteUntilStable().
  EXPECT_CALL(runtime_, HasInternalEvent())
      .WillOnce(Return(true))
      .WillOnce(Return(false));

  EXPECT_CALL(model_, GetEventlessTransitions(&runtime_))
      .WillOnce(Return(Transitions{&transition_AB}))
      .WillOnce(Return(Transitions()))
      .WillOnce(Return(Transitions{&transition_AB}))
      .WillOnce(Return(Transitions()));

  EXPECT_CALL(runtime_, DequeueInternalEvent())
      .WillOnce(Return(std::make_pair("I", "")));

  EXPECT_CALL(executor_, AssignEventData(&runtime_, "I", ""));

  EXPECT_CALL(model_, GetTransitionsForEvent(&runtime_, "I"))
      .WillOnce(Return(Transitions{&transition_BA}));

  InSequence sequence;

  EXPECT_CALL(runtime_, EraseActiveState(&state_A));
  EXPECT_CALL(runtime_, AddActiveState(&state_B));

  EXPECT_CALL(runtime_, EraseActiveState(&state_B));
  EXPECT_CALL(runtime_, AddActiveState(&state_A));

  EXPECT_CALL(runtime_, EraseActiveState(&state_A));
  EXPECT_CALL(runtime_, AddActiveState(&state_B));

  ExecuteUntilStable();
}

// Unhandled error will stop executing the macro step.
TEST_F(ExecutorTest, UnhandledErrorEventTerminatesExecuteUntilStable) {
  runtime_.SetRunning(true);

  EXPECT_CALL(model_, GetEventlessTransitions(&runtime_))
      .WillOnce(Return(Transitions()));
  EXPECT_CALL(runtime_, HasInternalEvent()).WillOnce(Return(true));
  EXPECT_CALL(runtime_, DequeueInternalEvent())
      .WillOnce(Return(std::make_pair("error", "")));
  EXPECT_CALL(executor_, AssignEventData(&runtime_, "error", ""));
  // No handlers.
  EXPECT_CALL(model_, GetTransitionsForEvent(&runtime_, "error"))
      .WillOnce(Return(Transitions()));

  // Never called.
  EXPECT_CALL(executor_, MicroStep(_, _, _)).Times(0);

  ExecuteUntilStable();
}

// Test that reaching a top level Final state sets running to false.
TEST_F(ExecutorTest, TopLevelFinalState) {
  // Model: A -> F
  // Event: none
  // Result: F
  MockState state_A("A");
  MockState state_F("F", true);
  MockTransition transition_AF(&state_A, &state_F);

  runtime_.SetRunning(true);

  InSequence sequence;

  EXPECT_CALL(model_, GetEventlessTransitions(&runtime_))
      .WillOnce(Return(Transitions{&transition_AF}));

  EXPECT_CALL(runtime_, EraseActiveState(&state_A));

  EXPECT_CALL(runtime_, AddActiveState(&state_F));

  EXPECT_CALL(runtime_, GetActiveStates()).WillOnce(Return(StateSet{&state_F}));

  EXPECT_CALL(runtime_, EraseActiveState(&state_F));

  ExecuteUntilStable();
  EXPECT_FALSE(runtime_.IsRunning());
}

// Test that entering the Final state of a compound state enqueues an internal
// event.
TEST_F(ExecutorTest, EnteringFinalOfCompoundStateEnqueuesEvent) {
  MockState state_A("A");
  MockState state_B("B", true);
  state_A.AddChild(&state_B);

  MockTransition transition(nullptr, &state_B);

  runtime_.SetRunning(true);

  InSequence sequence;

  EXPECT_CALL(runtime_, AddActiveState(&state_B));
  EXPECT_CALL(runtime_, EnqueueInternalEvent("done.state.A", ""));

  EnterStates(Transitions{&transition});
  EXPECT_TRUE(runtime_.IsRunning());
}

// Test that MicroStep executes the executables in the transition body.
TEST_F(ExecutorTest, MicroStepExecutesTransitionBody) {
  MockExecutableContent executable1;
  MockExecutableContent executable2;
  MockTransition transition1(nullptr, nullptr, &executable1);
  MockTransition transition2(nullptr, nullptr, &executable2);

  // Mock the exit/enter state calls.
  EXPECT_CALL(executor_, ExitStates(&model_, &runtime_, _));
  EXPECT_CALL(executor_, EnterStates(&model_, &runtime_, _));

  InSequence sequence;
  EXPECT_CALL(executable1, Execute(&runtime_));
  EXPECT_CALL(executable2, Execute(&runtime_));
  MicroStep({&transition1, &transition2});
}

// Test ExitStates runs the 'onexit' executable content.
TEST_F(ExecutorTest, ExitStateExecutesOnExit) {
  MockExecutableContent executable_A;
  MockExecutableContent executable_B;
  MockState state_A("A", false, false, nullptr, nullptr, &executable_A);
  MockState state_B("B", false, false, nullptr, nullptr, &executable_B);

  ON_CALL(model_, ComputeExitSet(&runtime_, _))
      .WillByDefault(Return(StateVec{&state_A, &state_B}));

  InSequence sequence;
  EXPECT_CALL(executable_A, Execute(&runtime_));
  EXPECT_CALL(executable_B, Execute(&runtime_));

  // We pass an empty transition list because we mocked ComputeExitSet to always
  // return state_A and state_B. A real call would include a set of transitions
  // but is not relevant to this test.
  ExitStates({});
}

// Test EnterStates runs the 'onenter' executable content and initial
// transitions' executable content.
TEST_F(ExecutorTest, EnterStateExecutesOnEnterAndInitial) {
  MockExecutableContent executable_A;
  MockExecutableContent executable_B;
  MockState state_A("A", false, false, nullptr, &executable_A);
  MockState state_B("B", false, false, nullptr, &executable_B);

  ON_CALL(model_, ComputeEntrySet(&runtime_, _, _, _))
      .WillByDefault(DoAll(SetArgPointee<2>(StateVec{&state_A, &state_B}),
                           Return(true)));

  {
    InSequence sequence;
    EXPECT_CALL(executable_A, Execute(&runtime_));
    EXPECT_CALL(executable_B, Execute(&runtime_));
  }
  // We pass an empty transition list because we mocked ComputeExitSet to always
  // return state_A and state_B. A real call would include a set of transitions
  // but is not relevant to this test.
  EnterStates({});

  LOG(INFO) << "Test that initial transition executable is called.";
  MockExecutableContent executable_C;
  MockState state_C("C", false, false, nullptr, &executable_C);
  MockExecutableContent executable_initial_C;
  MockTransition initial_transition_C(&state_C, &state_A,
                                      &executable_initial_C);
  EXPECT_TRUE(state_C.SetInitialTransition(&initial_transition_C));
  state_C.AddChild(&state_A);

  ON_CALL(model_, ComputeEntrySet(&runtime_, _, _, _))
      .WillByDefault(DoAll(SetArgPointee<2>(StateVec{&state_C, &state_A}),
                           SetArgPointee<3>(StateSet{&state_C}),
                           Return(true)));

  {
    InSequence sequence;
    EXPECT_CALL(executable_C, Execute(&runtime_));
    EXPECT_CALL(executable_initial_C, Execute(&runtime_));
    EXPECT_CALL(executable_A, Execute(&runtime_));
  }
  EnterStates({});
}

// Test that all datamodels are called at the start for early binding.
TEST_F(ExecutorTest, EarlyBindingDatamodelAtStart) {
  MockExecutableContent datamodel_A;
  MockExecutableContent datamodel_B;
  MockExecutableContent datamodel_C;
  MockExecutableContent datamodel_D;
  MockExecutableContent datamodel_E;
  MockExecutableContent datamodel_F;

  MockState state_A("A", false, false, &datamodel_A);
  MockState state_B("B", false, false, &datamodel_B);
  MockState state_C("C", false, false, &datamodel_C);
  MockState state_D("D", false, false, &datamodel_D);
  MockState state_E("E", false, false, &datamodel_E);
  MockState state_F("F", false, false, &datamodel_F);

  /*
  Document order: A C F B D E
   A      B
   |     / \
   C    D   E
   |
   F
  */
  state_A.AddChild(&state_C);
  state_B.AddChild(&state_D);
  state_B.AddChild(&state_E);
  state_C.AddChild(&state_F);

  MockExecutableContent datamodel_root;
  model_.SetDatamodelBlock(&datamodel_root);
  ON_CALL(model_, GetTopLevelStates())
      .WillByDefault(Return(StateVec{&state_A, &state_B}));

  // Only test datamodel initialization.
  ON_CALL(executor_, EnterStates(_, _, _)).WillByDefault(Return());
  ON_CALL(executor_, ExecuteUntilStable(_, _)).WillByDefault(Return());

  InSequence sequence;
  EXPECT_CALL(datamodel_root, Execute(&runtime_));
  EXPECT_CALL(datamodel_A, Execute(&runtime_));
  EXPECT_CALL(datamodel_C, Execute(&runtime_));
  EXPECT_CALL(datamodel_F, Execute(&runtime_));
  EXPECT_CALL(datamodel_B, Execute(&runtime_));
  EXPECT_CALL(datamodel_D, Execute(&runtime_));
  EXPECT_CALL(datamodel_E, Execute(&runtime_));

  Start();
}

// Test that internal "error.execution" events are enqueued when there is an
// error assigning the '_event' variable in the datamodel.
TEST_F(ExecutorTest, AssignEventDataEnqueuesErrorEventOnError) {
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              AssignString("_event.name", "E")).WillOnce(Return(false));

  EXPECT_CALL(runtime_, EnqueueInternalEvent("error.execution", _));

  executor_.AssignEventData(&runtime_, "E", "");

  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              AssignString("_event.name", "E")).WillOnce(Return(true));

  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              AssignExpression("_event.data", R"JSON({"text":"hello"})JSON"))
      .WillOnce(Return(false));

  EXPECT_CALL(runtime_, EnqueueInternalEvent("error.execution", _));

  executor_.AssignEventData(&runtime_, "E", R"JSON({"text":"hello"})JSON");
}

// Test that AssignEventData works as expected.
TEST_F(ExecutorTest, AssignEventDataSuccessful) {
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              AssignString("_event.name", "E")).WillOnce(Return(true));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              AssignExpression("_event.data", R"JSON({"text":"hello"})JSON"))
      .WillOnce(Return(true));

  executor_.AssignEventData(&runtime_, "E", R"JSON({"text":"hello"})JSON");
}

// Test that shutdown runs the 'onexit' executables in the correct order.
// Note that shutdown can occur when a state machine is canceled besides
// reaching a top-level final state.
TEST_F(ExecutorTest, ShutdownExecutesExitInOrder) {
  MockExecutableContent on_exit_A;
  MockExecutableContent on_exit_B;
  MockExecutableContent on_exit_C;

  /*
  Document order: A B C
     A
    / \
   B   C
  */
  MockState state_B("B", false, false, nullptr, nullptr, &on_exit_B);
  MockState state_C("C", false, false, nullptr, nullptr, &on_exit_C);
  MockState state_A("A", false, false, nullptr, nullptr, &on_exit_A);
  state_A.AddChild(&state_B);
  state_A.AddChild(&state_C);

  // Content should be executed and states deactivated in reverse document
  // order.
  EXPECT_CALL(on_exit_B, Execute(&runtime_)).Times(0);
  {
    InSequence sequence;
    EXPECT_CALL(runtime_, GetActiveStates())
        .WillOnce(Return(StateSet{&state_A, &state_C}));
    EXPECT_CALL(model_,
                SortStatesByDocumentOrder(
                    true, Pointee(UnorderedElementsAre(&state_A, &state_C))))
        .WillOnce(SetArgPointee<1>(StateVec{&state_C, &state_A}));
  }
  {
    InSequence sequence;
    EXPECT_CALL(on_exit_C, Execute(&runtime_));
    EXPECT_CALL(on_exit_A, Execute(&runtime_));
  }
  {
    InSequence sequence;
    EXPECT_CALL(runtime_, EraseActiveState(&state_C));
    EXPECT_CALL(runtime_, EraseActiveState(&state_A));
  }

  executor_.Shutdown(&model_, &runtime_);
}

TEST_F(ExecutorTest, EventDispatcher_StateEntered) {
  MockState state("A");
  MockTransition transition(nullptr, &state);

  EXPECT_CALL(runtime_.GetDefaultMockEventDispatcher(),
              NotifyStateEntered(&runtime_, &state));

  executor_.EnterStates(&model_, &runtime_, {&transition});
}

TEST_F(ExecutorTest, EventDispatcher_StateExited) {
  MockState state("A");
  MockTransition transition(&state, nullptr);

  EXPECT_CALL(runtime_.GetDefaultMockEventDispatcher(),
              NotifyStateExited(&runtime_, &state));

  executor_.ExitStates(&model_, &runtime_, {&transition});
}

TEST_F(ExecutorTest, EventDispatcher_TransitionFollowed) {
  MockTransition transition(nullptr, nullptr);

  EXPECT_CALL(runtime_.GetDefaultMockEventDispatcher(),
              NotifyTransitionFollowed(&runtime_, &transition));

  // Short-circuit the default behavior of EnterStates() and ExitStates().
  EXPECT_CALL(executor_, EnterStates(_, _, _)).WillOnce(Return());
  EXPECT_CALL(executor_, ExitStates(_, _, _)).WillOnce(Return());

  executor_.MicroStep(&model_, &runtime_, {&transition});
}

TEST_F(ExecutorTest,
       ParallelEnterLastFinalGrandchildEnqueuesInternalDoneEvent) {
  // Model:
  //      A       <- Parallel state
  // ____|||____
  // |    |    |
  // B    C    D  <- Compound states
  // |    |    |
  // E    F    G  <- Final states
  //
  // Suppose state G is entered and E, F are active states.
  // The done event should be emitted for A.

  MockState state_a("A", false, true);

  MockState state_b("B", false, false);
  MockState state_c("C", false, false);
  MockState state_d("D", false, false);

  MockState state_e("E", true);
  MockState state_f("F", true);
  MockState state_g("G", true);

  state_a.AddChildren({&state_b, &state_c, &state_d});
  state_b.AddChild(&state_e);
  state_c.AddChild(&state_f);
  state_d.AddChild(&state_g);

  MockTransition transition_dg(&state_d, &state_g, nullptr);

  ON_CALL(model_, ComputeEntrySet(&runtime_, Transitions{&transition_dg}, _, _))
      .WillByDefault(DoAll(SetArgPointee<2>(StateVec{&state_g}),
                           SetArgPointee<3>(StateSet{}),
                           Return(true)));
  ON_CALL(model_, IsInFinalState(&runtime_, &state_b))
      .WillByDefault(Return(true));
  ON_CALL(model_, IsInFinalState(&runtime_, &state_c))
      .WillByDefault(Return(true));
  ON_CALL(model_, IsInFinalState(&runtime_, &state_d))
      .WillByDefault(Return(true));

  InSequence sequence;
  EXPECT_CALL(runtime_, EnqueueInternalEvent("done.state.D", _))
      .WillOnce(Return());
  EXPECT_CALL(runtime_, EnqueueInternalEvent("done.state.A", _))
      .WillOnce(Return());

  executor_.EnterStates(&model_, &runtime_, {&transition_dg});
}

}  // namespace
}  // namespace state_chart
