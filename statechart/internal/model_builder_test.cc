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

#include "statechart/internal/model_builder.h"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "statechart/internal/model.h"
#include "statechart/internal/model/model.h"
#include "statechart/internal/testing/mock_executable_content.h"
#include "statechart/internal/testing/mock_runtime.h"
#include "statechart/internal/testing/mock_state.h"
#include "statechart/internal/testing/mock_transition.h"
#include "statechart/internal/testing/state_chart_builder.h"
#include "statechart/platform/map_util.h"
#include "statechart/platform/protobuf.h"
#include "statechart/platform/test_util.h"
#include "statechart/proto/state_chart.pb.h"

// The following tests must be written, once support for them has been added to
// ModelBuilder:
//
// 1) Building parallel states.
// 2) Building the final state.
// 5) Child states
// 7) Invoke
// 8) History
// 9) Initial ID/transition on a parallel state.

namespace state_chart {
namespace {

using ::gtl::InsertOrDie;
using ::gtl::InsertOrUpdate;
using ::testing::_;
using ::testing::DoAll;
using ::testing::ElementsAre;
using ::testing::EqualsProtobuf;
using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgPointee;

#define TEST_MODEL_BUILDER_DEFAULT(method) \
  ON_CALL(*this, method(_))                \
      .WillByDefault(testing::Invoke(this, &TestModelBuilder::Real##method));

#define TEST_MODEL_BUILDER_DELEGATE(return_type, arg_type, method) \
  return_type Real##method(arg_type e) { return ModelBuilder::method(e); }

class TestModelBuilder : public ModelBuilder {
 public:
  explicit TestModelBuilder(const config::StateChart& state_chart)
      : ModelBuilder(state_chart) {
    TEST_MODEL_BUILDER_DEFAULT(BuildExecutableContent);
    TEST_MODEL_BUILDER_DEFAULT(BuildExecutableBlock);
    TEST_MODEL_BUILDER_DEFAULT(BuildDataModelBlock);

    TEST_MODEL_BUILDER_DEFAULT(BuildRaise);
    TEST_MODEL_BUILDER_DEFAULT(BuildLog);
    TEST_MODEL_BUILDER_DEFAULT(BuildAssign);
    TEST_MODEL_BUILDER_DEFAULT(BuildSend);
    TEST_MODEL_BUILDER_DEFAULT(BuildIf);
    TEST_MODEL_BUILDER_DEFAULT(BuildForEach);

    TEST_MODEL_BUILDER_DEFAULT(BuildState);

    TEST_MODEL_BUILDER_DEFAULT(BuildTransitionsForState);
  }

  MOCK_METHOD1(BuildExecutableContent,
               model::ExecutableContent*(const config::ExecutableElement&));
  MOCK_METHOD1(BuildExecutableBlock,
               model::ExecutableContent*(
                   const proto2::RepeatedPtrField<config::ExecutableElement>&));
  MOCK_METHOD1(BuildDataModelBlock,
               model::ExecutableContent*(const config::DataModel&));

  MOCK_METHOD1(BuildRaise, model::Raise*(const config::Raise&));
  MOCK_METHOD1(BuildLog, model::Log*(const config::Log&));
  MOCK_METHOD1(BuildAssign, model::Assign*(const config::Assign&));
  MOCK_METHOD1(BuildSend, model::Send*(const config::Send&));
  MOCK_METHOD1(BuildIf, model::If*(const config::If&));
  MOCK_METHOD1(BuildForEach, model::ForEach*(const config::ForEach&));

  MOCK_METHOD1(BuildState, model::State*(const config::StateElement&));

  MOCK_METHOD1(BuildTransitionsForState, bool(model::State*));

  using ModelBuilder::all_elements_;
  using ModelBuilder::datamodel_block_;
  using ModelBuilder::initial_transition_;
  using ModelBuilder::states_config_map_;
  using ModelBuilder::states_map_;
  using ModelBuilder::top_level_states_;

 protected:
  // Delegates to parent (real) implementations.
  TEST_MODEL_BUILDER_DELEGATE(model::ExecutableContent*,
                              const config::ExecutableElement&,
                              BuildExecutableContent);
  TEST_MODEL_BUILDER_DELEGATE(
      model::ExecutableContent*,
      const proto2::RepeatedPtrField<config::ExecutableElement>&,
      BuildExecutableBlock);
  TEST_MODEL_BUILDER_DELEGATE(model::ExecutableContent*,
                              const config::DataModel&, BuildDataModelBlock);

  TEST_MODEL_BUILDER_DELEGATE(model::Raise*, const config::Raise&, BuildRaise);
  TEST_MODEL_BUILDER_DELEGATE(model::Log*, const config::Log&, BuildLog);
  TEST_MODEL_BUILDER_DELEGATE(model::Assign*, const config::Assign&,
                              BuildAssign);
  TEST_MODEL_BUILDER_DELEGATE(model::Send*, const config::Send&, BuildSend);
  TEST_MODEL_BUILDER_DELEGATE(model::If*, const config::If&, BuildIf);
  TEST_MODEL_BUILDER_DELEGATE(model::ForEach*, const config::ForEach&,
                              BuildForEach);
  TEST_MODEL_BUILDER_DELEGATE(model::State*, const config::StateElement&,
                              BuildState);

  TEST_MODEL_BUILDER_DELEGATE(bool, model::State*, BuildTransitionsForState);
};

class ModelBuilderTest : public ::testing::Test {
 protected:
  void Reset() { builder_.reset(new NiceMock<TestModelBuilder>(state_chart_)); }

  MockRuntime runtime_;

  config::StateChart state_chart_;
  std::unique_ptr<TestModelBuilder> builder_;
};

TEST_F(ModelBuilderTest, BuildExecutableBlock) {
  Reset();

  const model::ExecutableContent* block = nullptr;

  // Empty case.
  proto2::RepeatedPtrField<config::ExecutableElement> elements;
  block = builder_->BuildExecutableBlock(elements);
  ASSERT_EQ(nullptr, block);

  config::ExecutableBlockBuilder(&elements).AddRaise("foo").AddRaise("bar");

  MockExecutableContent executable_foo;
  MockExecutableContent executable_bar;

  {
    InSequence s;
    EXPECT_CALL(*builder_,
                BuildExecutableContent(EqualsProtobuf(elements.Get(0))))
        .WillOnce(Return(&executable_foo));
    EXPECT_CALL(*builder_,
                BuildExecutableContent(EqualsProtobuf(elements.Get(1))))
        .WillOnce(Return(&executable_bar));
  }

  block = builder_->BuildExecutableBlock(elements);
  ASSERT_NE(nullptr, block);

  // Show that the new executable runs both of the above individual executables.

  InSequence s;
  EXPECT_CALL(executable_foo, Execute(&runtime_));
  EXPECT_CALL(executable_bar, Execute(&runtime_));

  block->Execute(&runtime_);
}

TEST_F(ModelBuilderTest, BuildDataModelBlock) {
  Reset();

  config::DataModel data_model;

  // Empty case.
  const model::ExecutableContent* block = nullptr;
  block = builder_->BuildDataModelBlock(data_model);
  ASSERT_EQ(nullptr, block);

  config::DataModelBuilder(&data_model)
      .AddDataFromExpr("id1", "expr1")
      .AddDataFromSrc("id2", "expr2");

  block = builder_->BuildDataModelBlock(data_model);
  ASSERT_NE(nullptr, block);

  // Now show that the individual data assignment executables were correctly
  // created by running them against a MockRuntime.

  InSequence s;
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(), Declare("id1"))
      .WillOnce(Return(true));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              AssignExpression("id1", "expr1"))
      .WillOnce(Return(true));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(), Declare("id2"))
      .WillOnce(Return(true));
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              AssignExpression("id2", "expr2"))
      .WillOnce(Return(true));

  block->Execute(&runtime_);
}

TEST_F(ModelBuilderTest, BuildExecutableContent) {
  Reset();

  {  // Raise
    config::ExecutableElement e;
    e.mutable_raise()->set_event("event");
    model::Raise* expected = new model::Raise("event");
    EXPECT_CALL(*builder_, BuildRaise(EqualsProtobuf(e.raise())))
        .WillOnce(Return(expected));
    EXPECT_EQ(expected, builder_->BuildExecutableContent(e));
  }

  {  // Log
    config::ExecutableElement e;
    e.mutable_log()->set_expr("expr");
    model::Log* expected = new model::Log("", "expr");
    EXPECT_CALL(*builder_, BuildLog(EqualsProtobuf(e.log())))
        .WillOnce(Return(expected));
    EXPECT_EQ(expected, builder_->BuildExecutableContent(e));
  }

  {  // Assign
    config::ExecutableElement e;
    e.mutable_assign()->set_expr("expr");
    model::Assign* expected = new model::Assign("", "expr");
    EXPECT_CALL(*builder_, BuildAssign(EqualsProtobuf(e.assign())))
        .WillOnce(Return(expected));
    EXPECT_EQ(expected, builder_->BuildExecutableContent(e));
  }

  {  // Send
    using model::Expr;

    config::ExecutableElement e;
    auto* send = e.mutable_send();
    send->set_event("event");
    send->set_targetexpr("targetexpr");
    send->set_id("id");
    auto* expected = new model::Send("event", Expr("targetexpr"), "id", "");
    EXPECT_CALL(*builder_, BuildSend(EqualsProtobuf(e.send())))
        .WillOnce(Return(expected));
    EXPECT_EQ(expected, builder_->BuildExecutableContent(e));
  }

  {  // If
    config::ExecutableElement e;
    e.mutable_if_();
    auto* expected = new model::If({});
    EXPECT_CALL(*builder_, BuildIf(EqualsProtobuf(e.if_())))
        .WillOnce(Return(expected));
    EXPECT_EQ(expected, builder_->BuildExecutableContent(e));
  }

  {  // ForEach
    config::ExecutableElement e;
    e.mutable_foreach();
    auto* expected = new model::ForEach("", "", "", nullptr);
    EXPECT_CALL(*builder_, BuildForEach(EqualsProtobuf(e.foreach ())))
        .WillOnce(Return(expected));
    EXPECT_EQ(expected, builder_->BuildExecutableContent(e));
  }
}

TEST_F(ModelBuilderTest, BuildIf) {
  Reset();

  // We'll take advantage of ExecutableBlockBuilder to configure the If, since
  // it has a nice interface. We'll end up just using config[0] in the test.
  proto2::RepeatedPtrField<config::ExecutableElement> config;
  config::ExecutableBlockBuilder(&config)
      .AddIf("cond1")
      .AddRaise("raise1a")
      .AddRaise("raise1b")
      .ElseIf("cond2")
      .AddRaise("raise2")
      .Else()
      .AddRaise("raise3")
      .EndIf();

  const config::If& if_config = config.Get(0).if_();
  EXPECT_CALL(*builder_,
              BuildExecutableBlock(ElementsAre(
                  EqualsProtobuf(if_config.cond_executable(0).executable(0)),
                  EqualsProtobuf(if_config.cond_executable(0).executable(1)))));

  EXPECT_CALL(*builder_, BuildExecutableBlock(ElementsAre(EqualsProtobuf(
                             if_config.cond_executable(1).executable(0)))));

  EXPECT_CALL(*builder_, BuildExecutableBlock(ElementsAre(EqualsProtobuf(
                             if_config.cond_executable(2).executable(0)))));

  std::unique_ptr<model::If> if_executable(builder_->BuildIf(if_config));

  // FYI: We do not test the BuildIf() returned the correct instance of
  // model::If, only that the necessary calls to build its constructor arguments
  // took place (above), and that the returned pointer is non-null.
  ASSERT_NE(nullptr, if_executable);
}

// Test building an if statement with an empty executable block.
TEST_F(ModelBuilderTest, BuildIfWithEmptyExecutableBlock) {
  Reset();

  proto2::RepeatedPtrField<config::ExecutableElement> if_with_no_block;
  config::ExecutableBlockBuilder(&if_with_no_block).AddIf("cond1").EndIf();

  const config::If& if_config = if_with_no_block.Get(0).if_();
  std::unique_ptr<model::If> if_executable(builder_->BuildIf(if_config));

  ASSERT_NE(nullptr, if_executable);

  // Test the executable by executing it.
  EXPECT_CALL(runtime_.GetDefaultMockDatamodel(),
              EvaluateBooleanExpression("cond1", _))
      .WillOnce(DoAll(SetArgPointee<1>(true), Return(true)));

  EXPECT_TRUE(if_executable->Execute(&runtime_));
}

TEST_F(ModelBuilderTest, BuildForEach) {
  Reset();

  proto2::RepeatedPtrField<config::ExecutableElement> config;
  config::ExecutableBlockBuilder(&config)
      .AddForEach("array", "item", "index")
      .AddRaise("e1")
      .AddRaise("e2")
      .EndForEach();

  const config::ForEach& for_each_config = config.Get(0).foreach ();
  EXPECT_CALL(*builder_, BuildExecutableBlock(ElementsAre(
                             EqualsProtobuf(for_each_config.executable(0)),
                             EqualsProtobuf(for_each_config.executable(1)))));

  std::unique_ptr<model::ForEach> for_each_executable(
      builder_->BuildForEach(for_each_config));

  // FYI: We do not test the BuildForEach() returned the correct instance of
  // model::ForEach, only that the necessary calls to build its constructor
  // arguments took place (above), and that the returned pointer is non-null.
  ASSERT_NE(nullptr, for_each_executable);
}

// Test building a for each statement with an empty executable block body.
TEST_F(ModelBuilderTest, BuildForEachWithEmptyBody) {
  Reset();

  proto2::RepeatedPtrField<config::ExecutableElement> config;
  config::ExecutableBlockBuilder(&config)
      .AddForEach("[]", "item", "index")
      // Empty body.
      .EndForEach();

  const config::ForEach& for_each_config = config.Get(0).foreach ();
  EXPECT_CALL(*builder_, BuildExecutableBlock(ElementsAre()));

  std::unique_ptr<model::ForEach> for_each_executable(
      builder_->BuildForEach(for_each_config));

  ASSERT_NE(nullptr, for_each_executable);
}

TEST_F(ModelBuilderTest, BuildStateElement_State_Simple) {
  Reset();

  config::StateElement e;
  config::StateBuilder state_builder(e.mutable_state(), "id");

  const auto* state = builder_->BuildState(e);
  ASSERT_TRUE(state != nullptr);
  EXPECT_EQ("id", state->id());
  EXPECT_FALSE(state->IsFinal());
  EXPECT_THAT(state->GetTransitions(), ElementsAre());
  EXPECT_EQ(nullptr, state->GetInitialTransition());
  EXPECT_EQ(nullptr, state->GetParent());
  EXPECT_TRUE(state->IsAtomic());
  EXPECT_FALSE(state->IsCompound());
  EXPECT_EQ(nullptr, state->GetDatamodelBlock());
  EXPECT_EQ(nullptr, state->GetOnEntry());
  EXPECT_EQ(nullptr, state->GetOnExit());
}

TEST_F(ModelBuilderTest, BuildStateElement_State_ExecutableContent) {
  Reset();

  // Show creation of executable blocks for each of <datamodel>, <onentry>,
  // <onexit>.
  config::StateElement e;
  config::StateBuilder state_builder(e.mutable_state(), "id");
  state_builder.OnEntry().AddRaise("on_entry_event");
  state_builder.OnExit().AddRaise("on_exit_event");

  MockExecutableContent datamodel_executable;
  EXPECT_CALL(*builder_,
              BuildDataModelBlock(EqualsProtobuf(e.state().datamodel())))
      .WillOnce(Return(&datamodel_executable));

  MockExecutableContent on_entry_executable;
  EXPECT_CALL(*builder_, BuildExecutableBlock(
                             ElementsAre(EqualsProtobuf(e.state().onentry(0)))))
      .WillOnce(Return(&on_entry_executable));

  MockExecutableContent on_exit_executable;
  EXPECT_CALL(*builder_, BuildExecutableBlock(
                             ElementsAre(EqualsProtobuf(e.state().onexit(0)))))
      .WillOnce(Return(&on_exit_executable));

  const auto* state = builder_->BuildState(e);

  EXPECT_EQ(&datamodel_executable, state->GetDatamodelBlock());
  EXPECT_EQ(&on_entry_executable, state->GetOnEntry());
  EXPECT_EQ(&on_exit_executable, state->GetOnExit());
}

TEST_F(ModelBuilderTest, BuildStateElement_State_Final) {
  Reset();

  config::StateElement e;
  config::FinalStateBuilder state_builder(e.mutable_final(), "id");

  state_builder.OnEntry().AddRaise("event1");
  state_builder.OnExit().AddRaise("event2");

  MockExecutableContent on_entry_executable;
  EXPECT_CALL(*builder_, BuildExecutableBlock(
                             ElementsAre(EqualsProtobuf(e.final().onentry(0)))))
      .WillOnce(Return(&on_entry_executable));

  MockExecutableContent on_exit_executable;
  EXPECT_CALL(*builder_, BuildExecutableBlock(
                             ElementsAre(EqualsProtobuf(e.final().onexit(0)))))
      .WillOnce(Return(&on_exit_executable));

  const auto* state = builder_->BuildState(e);
  ASSERT_TRUE(state != nullptr);
  EXPECT_EQ("id", state->id());
  EXPECT_TRUE(state->IsFinal());
  EXPECT_THAT(state->GetTransitions(), ElementsAre());
  EXPECT_EQ(nullptr, state->GetInitialTransition());
  EXPECT_EQ(nullptr, state->GetParent());
  EXPECT_TRUE(state->IsAtomic());
  EXPECT_FALSE(state->IsCompound());
  EXPECT_EQ(nullptr, state->GetDatamodelBlock());

  EXPECT_EQ(&on_entry_executable, state->GetOnEntry());
  EXPECT_EQ(&on_exit_executable, state->GetOnExit());
}

TEST_F(ModelBuilderTest, Build) {
  config::StateChartBuilder sc_builder(&state_chart_, "test");

  sc_builder.DataModel().AddDataFromExpr("id", "expr");
  sc_builder.AddState("A");
  sc_builder.AddState("B");
  sc_builder.AddFinalState("F");

  Reset();

  MockExecutableContent datamodel_executable;
  EXPECT_CALL(*builder_,
              BuildDataModelBlock(EqualsProtobuf(state_chart_.datamodel())))
      .WillOnce(Return(&datamodel_executable));

  InSequence sequence;

  MockState state_A("A");
  EXPECT_CALL(*builder_, BuildState(EqualsProtobuf(state_chart_.state(0))))
      .WillOnce(Return(&state_A));

  MockState state_B("B");
  EXPECT_CALL(*builder_, BuildState(EqualsProtobuf(state_chart_.state(1))))
      .WillOnce(Return(&state_B));

  // Build() uses states_map_ to find all the states in the SC when building
  // transitions. If the states are not in the map, we won't build transitions.
  gtl::InsertOrDie(&builder_->states_map_, "A", &state_A);
  gtl::InsertOrDie(&builder_->states_map_, "B", &state_B);

  MockState state_F("F", true);
  EXPECT_CALL(*builder_, BuildState(EqualsProtobuf(state_chart_.state(2))))
      .WillOnce(Return(&state_F));

  // For each state, we also expect a call to build transitions for that state,
  // but only after the states have all been constructed.
  EXPECT_CALL(*builder_, BuildTransitionsForState(&state_A))
      .WillOnce(Return(true));
  EXPECT_CALL(*builder_, BuildTransitionsForState(&state_B))
      .WillOnce(Return(true));
  EXPECT_CALL(*builder_, BuildTransitionsForState(&state_F)).Times(0);

  builder_->Build();

  EXPECT_EQ(&datamodel_executable, builder_->datamodel_block_);
  EXPECT_THAT(builder_->top_level_states_,
              ElementsAre(&state_A, &state_B, &state_F));
}

TEST_F(ModelBuilderTest, Build_InitialTransition) {
  config::StateChartBuilder sc_builder(&state_chart_, "test");

  sc_builder.AddState("A");
  sc_builder.AddState("B");

  Reset();

  // The default is to make the first state in document order be the initial
  // transition state.
  builder_->Build();

  ASSERT_NE(nullptr, builder_->initial_transition_);
  EXPECT_EQ(nullptr, builder_->initial_transition_->GetSourceState());
  const auto& targets = builder_->initial_transition_->GetTargetStates();
  EXPECT_EQ(1, targets.size());
  EXPECT_EQ("A", targets[0]->id());

  // This time explicitly set the initial to "B".
  sc_builder.AddInitial("B");

  Reset();
  builder_->Build();

  ASSERT_NE(nullptr, builder_->initial_transition_);
  EXPECT_EQ(nullptr, builder_->initial_transition_->GetSourceState());
  const auto& targets2 = builder_->initial_transition_->GetTargetStates();
  EXPECT_EQ(1, targets2.size());
  EXPECT_EQ("B", targets2[0]->id());
}

TEST_F(ModelBuilderTest, BuildTransitionsForState) {
  // Tests missing from this case:
  //
  // 1) Transition type (internal/external)
  // 2) Transition conditions
  // 3) Transition to self (explicit)
  // 4) Transition to self (implicit)
  // 5) Sub-states
  // 6) Initial transition
  // 7) Empty executable.
  Reset();

  // BuildTransitionsForState() uses the states_config_map_ to retrieve the
  // original config element, and then iterates through each transition config.

  // Empty case first.
  MockState state_A("A");

  config::StateElement e;
  gtl::InsertOrUpdate(&builder_->states_config_map_, "A", &e);

  builder_->BuildTransitionsForState(&state_A);
  EXPECT_THAT(state_A.GetTransitions(), ElementsAre());

  config::StateBuilder s_builder(e.mutable_state(), "A");
  s_builder.AddTransition({"t1_event1", "t1_event2"}, {"B", "C"})
      .AddRaise("t1_foo")
      .AddRaise("t1_bar");
  s_builder.AddTransition({"t2_event1"}, {"D"}, "some_condition")
      .AddRaise("t2_foo");

  gtl::InsertOrUpdate(&builder_->states_config_map_, "A", &e);

  MockState state_B("B");
  MockState state_C("C");
  MockState state_D("D");

  gtl::InsertOrDie(&builder_->states_map_, "B", &state_B);
  gtl::InsertOrDie(&builder_->states_map_, "C", &state_C);
  gtl::InsertOrDie(&builder_->states_map_, "D", &state_D);

  MockExecutableContent t1_executable;
  EXPECT_CALL(*builder_,
              BuildExecutableBlock(ElementsAre(
                  EqualsProtobuf(e.state().transition(0).executable(0)),
                  EqualsProtobuf(e.state().transition(0).executable(1)))))
      .WillOnce(Return(&t1_executable));

  MockExecutableContent t2_executable;
  EXPECT_CALL(*builder_, BuildExecutableBlock(ElementsAre(EqualsProtobuf(
                             e.state().transition(1).executable(0)))))
      .WillOnce(Return(&t2_executable));

  builder_->BuildTransitionsForState(&state_A);

  const auto& transitions = state_A.GetTransitions();
  EXPECT_EQ(2, transitions.size());

  EXPECT_EQ(&state_A, transitions[0]->GetSourceState());
  EXPECT_EQ("", transitions[0]->GetCondition());
  EXPECT_THAT(transitions[0]->GetTargetStates(),
              ElementsAre(&state_B, &state_C));
  EXPECT_THAT(transitions[0]->GetEvents(),
              ElementsAre("t1_event1", "t1_event2"));
  EXPECT_EQ(&t1_executable, transitions[0]->GetExecutable());

  EXPECT_EQ(&state_A, transitions[1]->GetSourceState());
  EXPECT_THAT(transitions[1]->GetTargetStates(), ElementsAre(&state_D));
  EXPECT_THAT(transitions[1]->GetEvents(), ElementsAre("t2_event1"));
  EXPECT_EQ("some_condition", transitions[1]->GetCondition());
  EXPECT_EQ(&t2_executable, transitions[1]->GetExecutable());
}

TEST_F(ModelBuilderTest, BuildTransitionsForState_StripEventWildcards) {
  // Show that we correctly strip suffixes of ".*" and ".", but leave the
  // special wild-card event "*" alone.
  Reset();

  MockState state_A("A");

  config::StateElement e;
  config::StateBuilder s_builder(e.mutable_state(), "A");
  s_builder.AddTransition({"event1.foo", "event2.", "event3.*", "*"}, {});

  gtl::InsertOrUpdate(&builder_->states_config_map_, "A", &e);

  builder_->BuildTransitionsForState(&state_A);
  const auto& transitions = state_A.GetTransitions();
  ASSERT_EQ(1, transitions.size());

  EXPECT_THAT(transitions[0]->GetEvents(),
              ElementsAre("event1.foo", "event2", "event3", "*"));
}

TEST_F(ModelBuilderTest, BuildCompoundState) {
  config::StateElement e;
  config::StateBuilder s_builder(e.mutable_state(), "A");

  s_builder.AddState("B");
  s_builder.AddState("C");

  model::State* state_A = nullptr;

  Reset();
  state_A = builder_->BuildState(e);
  // Check that the children are correct.
  ASSERT_EQ(2, state_A->GetChildren().size());
  EXPECT_EQ("B", state_A->GetChildren()[0]->id());
  EXPECT_EQ("C", state_A->GetChildren()[1]->id());
  // Check that initial transition is correct.
  ASSERT_EQ(1, state_A->GetInitialTransition()->GetTargetStates().size());
  EXPECT_EQ(state_A->GetInitialTransition()->GetTargetStates()[0],
            state_A->GetChildren()[0]);

  // Switch the initial transition.
  s_builder.AddInitialId("C");
  Reset();
  state_A = builder_->BuildState(e);
  // Check that the children are correct.
  ASSERT_EQ(2, state_A->GetChildren().size());
  EXPECT_EQ("B", state_A->GetChildren()[0]->id());
  EXPECT_EQ("C", state_A->GetChildren()[1]->id());
  // Check that initial transition is correct.
  ASSERT_EQ(1, state_A->GetInitialTransition()->GetTargetStates().size());
  EXPECT_EQ(state_A->GetInitialTransition()->GetTargetStates()[0],
            state_A->GetChildren()[1]);
}

TEST_F(ModelBuilderTest, CreateModelAndReset) {
  // Show that when CreateModelAndReset() is called, we return a new ModelImpl
  // with the states, initial transition, datamodel_binding and datamodel
  // executables.
  config::StateChartBuilder sc_builder(&state_chart_, "test");

  sc_builder.DataModel().AddDataFromExpr("id", "expr");

  sc_builder.AddState("A");
  sc_builder.AddState("B");

  Reset();

  MockState state_A("A");
  EXPECT_CALL(*builder_, BuildState(EqualsProtobuf(state_chart_.state(0))))
      .WillOnce(Return(&state_A));

  MockState state_B("B");
  EXPECT_CALL(*builder_, BuildState(EqualsProtobuf(state_chart_.state(1))))
      .WillOnce(Return(&state_B));

  MockExecutableContent datamodel_executable;
  EXPECT_CALL(*builder_,
              BuildDataModelBlock(EqualsProtobuf(state_chart_.datamodel())))
      .WillOnce(Return(&datamodel_executable));

  // Prevent default delegation to parent for creating transitions.
  EXPECT_CALL(*builder_, BuildTransitionsForState(_))
      .WillRepeatedly(Return(true));

  builder_->Build();

  std::unique_ptr<Model> model(builder_->CreateModelAndReset());

  EXPECT_EQ(config::StateChart::BINDING_EARLY, model->GetDatamodelBinding());
  EXPECT_EQ(&datamodel_executable, model->GetDatamodelBlock());
  EXPECT_THAT(model->GetTopLevelStates(), ElementsAre(&state_A, &state_B));
  EXPECT_THAT(model->GetInitialTransition()->GetTargetStates(),
              ElementsAre(&state_A));
}

TEST_F(ModelBuilderTest, BuildParallelState) {
  config::StateElement e;
  config::ParallelBuilder s_builder(e.mutable_parallel(), "A");

  s_builder.AddState("B");
  s_builder.AddState("C");

  model::State* state_A = nullptr;

  Reset();
  state_A = builder_->BuildState(e);
  // Check that the children are correct.
  ASSERT_EQ(2, state_A->GetChildren().size());
  EXPECT_EQ("B", state_A->GetChildren()[0]->id());
  EXPECT_EQ("C", state_A->GetChildren()[1]->id());
  // Check that initial transition is correct.
  ASSERT_EQ(2, state_A->GetInitialTransition()->GetTargetStates().size());
  EXPECT_EQ(state_A->GetInitialTransition()->GetTargetStates(),
            state_A->GetChildren());
}

TEST_F(ModelBuilderTest, BuildStateElement_Parallel_ExecutableContent) {
  Reset();

  // Show creation of executable blocks for each of <datamodel>, <onentry>,
  // <onexit>.
  config::StateElement e;
  config::ParallelBuilder state_builder(e.mutable_parallel(), "id");
  state_builder.OnEntry().AddRaise("on_entry_event");
  state_builder.OnExit().AddRaise("on_exit_event");

  MockExecutableContent datamodel_executable;
  EXPECT_CALL(*builder_,
              BuildDataModelBlock(EqualsProtobuf(e.parallel().datamodel())))
      .WillOnce(Return(&datamodel_executable));

  MockExecutableContent on_entry_executable;
  EXPECT_CALL(*builder_, BuildExecutableBlock(ElementsAre(
                             EqualsProtobuf(e.parallel().onentry(0)))))
      .WillOnce(Return(&on_entry_executable));

  MockExecutableContent on_exit_executable;
  EXPECT_CALL(
      *builder_,
      BuildExecutableBlock(ElementsAre(EqualsProtobuf(e.parallel().onexit(0)))))
      .WillOnce(Return(&on_exit_executable));

  const auto* state = builder_->BuildState(e);

  EXPECT_EQ(&datamodel_executable, state->GetDatamodelBlock());
  EXPECT_EQ(&on_entry_executable, state->GetOnEntry());
  EXPECT_EQ(&on_exit_executable, state->GetOnExit());
}
}  // namespace
}  // namespace state_chart
