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

#include "statechart/internal/model_impl.h"

#include <algorithm>
#include <vector>

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "statechart/platform/test_util.h"
#include "statechart/internal/model/model.h"
#include "statechart/internal/testing/mock_runtime.h"
#include "statechart/internal/testing/mock_state.h"
#include "statechart/internal/testing/mock_transition.h"

using proto2::contrib::parse_proto::ParseTextOrDie;
using testing::_;
using testing::ContainerEq;
using testing::ElementsAre;
using testing::Return;
using testing::UnorderedElementsAre;

namespace state_chart {
namespace {

// A class to expose the selected protected members of ModelImpl for testing.
class TestModelImpl : public ModelImpl {
 public:
  TestModelImpl(const string& name, const model::Transition* initial_transition,
                const std::vector<const model::State*>& top_level_states,
                config::StateChart::Binding datamodel_binding,
                const model::ExecutableContent* datamodel,
                const std::vector<const model::ModelElement*>& model_elements)
      : ModelImpl(name, initial_transition, top_level_states, datamodel_binding,
                  datamodel, model_elements) {}

  using ModelImpl::StateDocumentOrderLessThan;
  using ModelImpl::SelectTransitions;
  using ModelImpl::RemoveConflictingTransitions;
};

class ModelImplTest : public ::testing::Test {
 protected:
  ModelImplTest() {}

  // Creates a simple model with initial transition.
  void Reset(const std::vector<const model::State*>& top_level_states,
             const model::Transition* initial_transition) {
    model_.reset(new TestModelImpl("", initial_transition, top_level_states,
                                   config::StateChart::BINDING_EARLY, nullptr,
                                   {}));
  }

  // Creates a simple model without initial transition.
  void Reset(const std::vector<const model::State*>& top_level_states) {
    return Reset(top_level_states, nullptr);
  }

  std::vector<const model::Transition*> SelectTransitions(const string* event) {
    return model_->SelectTransitions(&runtime_, event);
  }

  std::unique_ptr<TestModelImpl> model_;
  MockRuntime runtime_;
};

// Test that the document ordering function for states is correct.
TEST_F(ModelImplTest, TestStateDocumentOrderAndSorting) {
  MockState state_A("A");
  MockState state_B("B");
  MockState state_C("C");
  MockState state_D("D");
  MockState state_E("E");
  MockState state_F("F");

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

  Reset({&state_A, &state_B});

  LOG(INFO) << "Test top level states and equality.";
  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_A, &state_B));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_A, &state_A));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_B, &state_A));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_B, &state_B));

  LOG(INFO) << "Test ancestor descendants.";
  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_A, &state_C));
  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_A, &state_F));
  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_C, &state_F));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_C, &state_A));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_F, &state_A));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_F, &state_C));

  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_B, &state_D));
  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_B, &state_E));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_D, &state_B));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_E, &state_B));

  LOG(INFO) << "Test across branches.";
  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_A, &state_D));
  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_A, &state_E));

  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_C, &state_B));
  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_C, &state_D));
  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_C, &state_E));

  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_F, &state_B));
  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_F, &state_D));
  EXPECT_TRUE(model_->StateDocumentOrderLessThan(&state_F, &state_E));

  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_D, &state_A));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_E, &state_A));

  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_B, &state_C));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_D, &state_C));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_E, &state_C));

  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_B, &state_F));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_D, &state_F));
  EXPECT_FALSE(model_->StateDocumentOrderLessThan(&state_E, &state_F));

  // Test sorting.
  const std::vector<const model::State*> document_order{
      &state_A, &state_C, &state_F, &state_B, &state_D, &state_E};
  std::vector<const model::State*> state_permutation;

  LOG(INFO) << "Test document order sorting.";
  state_permutation = {&state_D, &state_E, &state_F,
                       &state_A, &state_B, &state_C};
  model_->SortStatesByDocumentOrder(false, &state_permutation);
  EXPECT_THAT(state_permutation, ContainerEq(document_order));

  LOG(INFO) << "Test reverse document order sorting.";
  state_permutation = {&state_C, &state_E, &state_A,
                       &state_F, &state_B, &state_D};
  model_->SortStatesByDocumentOrder(true, &state_permutation);
  std::reverse(state_permutation.begin(), state_permutation.end());
  EXPECT_THAT(state_permutation, ContainerEq(document_order));
}

// Test selecting transitions for top-level active states.
TEST_F(ModelImplTest, SelectTransitionTopLevel) {
  MockState state_A("A");
  MockState state_B("B");
  MockState state_C("C");

  MockTransition transition_AB(&state_A, &state_B);  // eventless
  MockTransition transition_AC(&state_A, &state_C, {"event"});
  state_A.mutable_transitions()->push_back(&transition_AC);
  state_A.mutable_transitions()->push_back(&transition_AB);

  MockTransition transition_BC(&state_B, &state_C);  // eventless
  MockTransition transition_BA(&state_B, &state_A, {"event"});
  state_B.mutable_transitions()->push_back(&transition_BC);
  state_B.mutable_transitions()->push_back(&transition_BA);

  Reset({&state_A, &state_B, &state_C});

  // Setup the four test cases.
  EXPECT_CALL(runtime_, GetActiveStates())
      .WillOnce(Return(StateSet{&state_A}))
      .WillOnce(Return(StateSet{&state_B}))
      .WillOnce(Return(StateSet{&state_A}))
      .WillOnce(Return(StateSet{&state_B}));

  LOG(INFO) << "Test eventless.";
  EXPECT_THAT(SelectTransitions(nullptr), ElementsAre(&transition_AB));
  EXPECT_THAT(SelectTransitions(nullptr), ElementsAre(&transition_BC));

  LOG(INFO) << "Test with event and event hierarchy.";
  string event = "event";
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition_AC));
  event = "event.something.somewhat";
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition_BA));
}

// Test selecting transitions with conditions for top-level active states.
TEST_F(ModelImplTest, SelectTransitionTopLevelWithConditions) {
  MockState state_A("A");
  MockState state_B("B");
  MockState state_C("C");

  MockTransition transition_AB(&state_A, &state_B);
  MockTransition transition_AC(&state_A, &state_C);
  state_A.mutable_transitions()->push_back(&transition_AB);
  state_A.mutable_transitions()->push_back(&transition_AC);

  MockTransition transition_BC(&state_B, &state_C, {"event"});
  MockTransition transition_BA(&state_B, &state_A, {"event"});
  state_B.mutable_transitions()->push_back(&transition_BC);
  state_B.mutable_transitions()->push_back(&transition_BA);

  Reset({&state_A, &state_B, &state_C});

  // Setup the four test cases.
  EXPECT_CALL(runtime_, GetActiveStates())
      .WillOnce(Return(StateSet{&state_A}))
      .WillOnce(Return(StateSet{&state_A}))
      .WillOnce(Return(StateSet{&state_B}))
      .WillOnce(Return(StateSet{&state_B}));

  LOG(INFO) << "Test conditions for eventless.";
  EXPECT_CALL(transition_AB, EvaluateCondition(&runtime_))
      .WillOnce(Return(false));
  EXPECT_THAT(SelectTransitions(nullptr), ElementsAre(&transition_AC));
  EXPECT_CALL(transition_AB, EvaluateCondition(&runtime_))
      .WillOnce(Return(true));
  EXPECT_THAT(SelectTransitions(nullptr), ElementsAre(&transition_AB));

  LOG(INFO) << "Test conditions with events.";
  string event = "event";
  EXPECT_CALL(transition_BC, EvaluateCondition(&runtime_))
      .WillOnce(Return(false));
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition_BA));
  EXPECT_CALL(transition_BC, EvaluateCondition(&runtime_))
      .WillOnce(Return(true));
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition_BC));
}

// Test event matching.
TEST_F(ModelImplTest, SelectTransitionEventMatching) {
  MockState state_A("A");
  MockState state_B("B");

  MockTransition transition1(&state_A, &state_B, {"event1"});
  MockTransition transition2(&state_A, &state_B,
                             {"event1.subevent.subsubevent"});
  MockTransition transition3(&state_A, &state_B, {"event2"});
  MockTransition transition4(&state_A, &state_B, {"event3", "event4"});
  state_A.mutable_transitions()->assign(
      {&transition1, &transition2, &transition3, &transition4});

  Reset({&state_A, &state_B});

  ON_CALL(runtime_, GetActiveStates())
      .WillByDefault(Return(StateSet{&state_A}));

  string event;

  LOG(INFO) << "Test no matching transition.";
  event = "no_match_event";
  EXPECT_TRUE(SelectTransitions(&event).empty());

  LOG(INFO) << "Test correct transition chosen.";
  event = "event1";
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition1));
  event = "event2.subevent";
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition3));
  event = "event4.subevent.subsubevent";
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition4));
  event = "event3";
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition4));

  LOG(INFO) << "Test first matching transition chosen when multiple match.";
  event = "event1.subevent.subsubevent";
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition1));

  LOG(INFO) << "Test wildcard.";
  MockTransition transition5(&state_A, &state_B, {"*"});
  state_A.mutable_transitions()->assign({&transition1, &transition5});

  event = "event1";
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition1));
  event = "a_very_wild_event";
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition5));
}

// Test selecting transitions for compound active states.
TEST_F(ModelImplTest, SelectTransitionInCompoundStates) {
  /*
  Document order: A B C D
   A   D
   |
   B
   |
   C
  */
  MockState state_A("A");
  MockState state_B("B");
  state_A.AddChild(&state_B);
  MockState state_C("C");
  state_B.AddChild(&state_C);

  MockState state_D("D");

  MockTransition transition_AD(&state_A, &state_D, {"event1"});
  state_A.mutable_transitions()->push_back(&transition_AD);

  MockTransition transition_BD(&state_B, &state_D, {"event2"});
  state_B.mutable_transitions()->push_back(&transition_BD);

  Reset({&state_A, &state_D});

  // Create expectation for the test case calls.
  EXPECT_CALL(runtime_, GetActiveStates())
      .Times(2)
      .WillRepeatedly(Return(StateSet{&state_A, &state_B, &state_C}));

  string event;

  LOG(INFO) << "Test from state C.";
  event = "event1";
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition_AD));
  event = "event2";
  EXPECT_THAT(SelectTransitions(&event), ElementsAre(&transition_BD));
}

// Test compute entry order for initial transition.
TEST_F(ModelImplTest, ComputeEntryOrderForInitialTransition) {
  MockState state_A("A");
  MockTransition initial_transition(nullptr, &state_A);
  Reset({&state_A}, &initial_transition);

  std::vector<const model::State*> states_to_enter;
  std::set<const model::State*> states_for_default_entry;
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&initial_transition},
                                      &states_to_enter,
                                      &states_for_default_entry));
  EXPECT_THAT(states_to_enter, ElementsAre(&state_A));
  EXPECT_TRUE(states_for_default_entry.empty());
}

// Test compute entry order for flat states.
TEST_F(ModelImplTest, ComputeEntryOrderForFlatStates) {
  LOG(INFO) << "Test common cases with transition source and targets.";
  MockState state_A("A");
  MockState state_B("B");

  MockTransition transition_AB(&state_A, &state_B, {"event1"});
  state_A.mutable_transitions()->push_back(&transition_AB);

  MockTransition transition_BA(&state_B, &state_A, {"event2"});
  state_B.mutable_transitions()->push_back(&transition_BA);

  Reset({&state_A, &state_B});

  std::vector<const model::State*> states_to_enter;
  std::set<const model::State*> states_for_default_entry;

  states_to_enter.clear();
  states_for_default_entry.clear();
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&transition_AB},
                                      &states_to_enter,
                                      &states_for_default_entry));
  EXPECT_THAT(states_to_enter, ElementsAre(&state_B));
  EXPECT_TRUE(states_for_default_entry.empty());

  states_to_enter.clear();
  states_for_default_entry.clear();
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&transition_BA},
                                      &states_to_enter,
                                      &states_for_default_entry));
  EXPECT_THAT(states_to_enter, ElementsAre(&state_A));
  EXPECT_TRUE(states_for_default_entry.empty());

  LOG(INFO) << "Test transition target == source.";
  MockTransition transition_AA(&state_A, &state_A, {"event"});
  state_A.mutable_transitions()->assign({&transition_AA});
  Reset({&state_A});

  states_to_enter.clear();
  states_for_default_entry.clear();
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&transition_AA},
                                      &states_to_enter,
                                      &states_for_default_entry));
  EXPECT_THAT(states_to_enter, ElementsAre(&state_A));
  EXPECT_TRUE(states_for_default_entry.empty());

  LOG(INFO) << "Test transition target unspecified.";
  MockTransition transition_AA_no_target(&state_A, nullptr, {"event"});
  state_A.mutable_transitions()->assign({&transition_AA_no_target});
  Reset({&state_A});

  states_to_enter.clear();
  states_for_default_entry.clear();
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&transition_AA_no_target},
                                      &states_to_enter,
                                      &states_for_default_entry));
  EXPECT_TRUE(states_to_enter.empty());
  EXPECT_TRUE(states_for_default_entry.empty());
}

// Test compute entry order for compound states.
TEST_F(ModelImplTest, ComputeEntryOrderForCompoundStates) {
  MockState state_D("D");
  MockState state_E("E");

  MockState state_B("B", false);
  MockTransition initial_BD(&state_B, &state_D);
  EXPECT_TRUE(state_B.SetInitialTransition(&initial_BD));

  MockState state_F("F");

  MockState state_C("C", false);
  MockTransition initial_FC(&state_C, &state_F);
  EXPECT_TRUE(state_C.SetInitialTransition(&initial_FC));

  MockState state_A("A", false);
  MockTransition initial_AC(&state_A, &state_C);
  EXPECT_TRUE(state_A.SetInitialTransition(&initial_AC));

  /*
  Document order: A C F B D E
   A      B
   |     / \
   C    D   E
   |
   F
  */
  // Initial transitions are to first child of each state in document order.
  state_A.AddChild(&state_C);
  state_B.AddChild(&state_D);
  state_B.AddChild(&state_E);
  state_C.AddChild(&state_F);

  Reset({&state_A, &state_B});

  std::vector<const model::State*> states_to_enter;
  std::set<const model::State*> states_for_default_entry;

  // External transition from F to E.
  MockTransition transition_FE(&state_F, &state_E, {"event_FE"});
  states_to_enter.clear();
  states_for_default_entry.clear();
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&transition_FE},
                                      &states_to_enter,
                                      &states_for_default_entry));
  EXPECT_THAT(states_to_enter, ElementsAre(&state_B, &state_E));
  // No default entry -- no initial transitions taken.
  EXPECT_TRUE(states_for_default_entry.empty());

  // External transition from F to B.
  MockTransition transition_FB(&state_F, &state_B, {"event_FB"});
  states_to_enter.clear();
  states_for_default_entry.clear();
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&transition_FB},
                                      &states_to_enter,
                                      &states_for_default_entry));
  EXPECT_THAT(states_to_enter, ElementsAre(&state_B, &state_D));
  EXPECT_THAT(states_for_default_entry, UnorderedElementsAre(&state_B));

  // External transition from E to D.
  MockTransition transition_ED(&state_E, &state_D, {"event_ED"});
  states_to_enter.clear();
  states_for_default_entry.clear();
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&transition_ED},
                                      &states_to_enter,
                                      &states_for_default_entry));
  // The least common compound ancestor (LCCA) state_B is not entered.
  EXPECT_THAT(states_to_enter, ElementsAre(&state_D));
  EXPECT_TRUE(states_for_default_entry.empty());

  // External transition from B to A.
  MockTransition transition_BA(&state_B, &state_A, {"event_BA"});
  states_to_enter.clear();
  states_for_default_entry.clear();
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&transition_BA},
                                      &states_to_enter,
                                      &states_for_default_entry));
  EXPECT_THAT(states_to_enter, ElementsAre(&state_A, &state_C, &state_F));
  EXPECT_THAT(states_for_default_entry,
              UnorderedElementsAre(&state_A, &state_C));

  // Internal transition from C to F. This is the special case where the target,
  // F, is a descendant of the source state C. The internal transition will not
  // exit and re-enter the source state C, but an external transition will.
  MockTransition transition_CF_internal(&state_C, {&state_F}, {"event_CF"}, "",
                                        true);
  states_to_enter.clear();
  states_for_default_entry.clear();
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&transition_CF_internal},
                                      &states_to_enter,
                                      &states_for_default_entry));
  EXPECT_THAT(states_to_enter, ElementsAre(&state_F));
  EXPECT_TRUE(states_for_default_entry.empty());

  // External transition from C to F. C should be exited and re-entered.
  // Note that C's initial transition should not be taken.
  MockTransition transition_CF(&state_C, &state_F, {"event_CF"});
  states_to_enter.clear();
  states_for_default_entry.clear();
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&transition_CF},
                                      &states_to_enter,
                                      &states_for_default_entry));
  EXPECT_THAT(states_to_enter, ElementsAre(&state_C, &state_F));
  EXPECT_TRUE(states_for_default_entry.empty());
}

// Test compute exit order for flat states.
TEST_F(ModelImplTest, ComputeExitOrderForFlatStates) {
  LOG(INFO) << "Test common cases with transition source and targets.";
  MockState state_A("A");
  MockState state_B("B");

  MockTransition transition_AB(&state_A, &state_B, {"event1"});
  state_A.mutable_transitions()->push_back(&transition_AB);

  MockTransition transition_BA(&state_B, &state_A, {"event2"});
  state_B.mutable_transitions()->push_back(&transition_BA);

  Reset({&state_A, &state_B});

  // Mock calls for test cases.
  EXPECT_CALL(runtime_, GetActiveStates())
      .WillOnce(Return(StateSet{&state_A}))
      .WillOnce(Return(StateSet{&state_B}));

  std::vector<const model::State*> states_to_exit;

  states_to_exit = model_->ComputeExitSet(&runtime_, {&transition_AB});
  EXPECT_THAT(states_to_exit, ElementsAre(&state_A));

  states_to_exit = model_->ComputeExitSet(&runtime_, {&transition_BA});
  EXPECT_THAT(states_to_exit, ElementsAre(&state_B));

  LOG(INFO) << "Test transition target == source.";
  MockTransition transition_AA(&state_A, &state_A, {"event"});
  state_A.mutable_transitions()->assign({&transition_AA});
  Reset({&state_A});

  EXPECT_CALL(runtime_, GetActiveStates()).WillOnce(Return(StateSet{&state_A}));

  states_to_exit = model_->ComputeExitSet(&runtime_, {&transition_AA});
  EXPECT_THAT(states_to_exit, ElementsAre(&state_A));

  LOG(INFO) << "Test transition target unspecified.";
  MockTransition transition_AA_no_target(&state_A, nullptr, {"event"});
  state_A.mutable_transitions()->assign({&transition_AA_no_target});
  Reset({&state_A});

  EXPECT_CALL(runtime_, GetActiveStates()).WillOnce(Return(StateSet{&state_A}));

  states_to_exit =
      model_->ComputeExitSet(&runtime_, {&transition_AA_no_target});
  EXPECT_TRUE(states_to_exit.empty());
}

// Test compute exit order for compound states.
TEST_F(ModelImplTest, ComputeExitOrderForCompoundStates) {
  MockState state_D("D");
  MockState state_E("E");

  MockState state_B("B", false);
  MockTransition initial_BD(&state_B, &state_D);
  EXPECT_TRUE(state_B.SetInitialTransition(&initial_BD));

  MockState state_F("F");

  MockState state_C("C", false);
  MockTransition initial_FC(&state_C, &state_F);
  EXPECT_TRUE(state_C.SetInitialTransition(&initial_FC));

  MockState state_A("A", false);
  MockTransition initial_AC(&state_A, &state_C);
  EXPECT_TRUE(state_A.SetInitialTransition(&initial_AC));

  /*
  Document order: A C F B D E
   A      B
   |     / \
   C    D   E
   |
   F
  */
  // Initial transitions are to first child of each state in document order.
  state_A.AddChild(&state_C);
  state_B.AddChild(&state_D);
  state_B.AddChild(&state_E);
  state_C.AddChild(&state_F);

  Reset({&state_A, &state_B});

  std::vector<const model::State*> states_to_exit;

  // External transition from F to E.
  MockTransition transition_FE(&state_F, &state_E, {"event_FE"});
  EXPECT_CALL(runtime_, GetActiveStates())
      .WillOnce(Return(StateSet{&state_F, &state_C, &state_A}));
  states_to_exit = model_->ComputeExitSet(&runtime_, {&transition_FE});
  EXPECT_THAT(states_to_exit, ElementsAre(&state_F, &state_C, &state_A));

  // External transition from F to B.
  MockTransition transition_FB(&state_F, &state_B, {"event_FB"});
  EXPECT_CALL(runtime_, GetActiveStates())
      .WillOnce(Return(StateSet{&state_F, &state_C, &state_A}));
  states_to_exit = model_->ComputeExitSet(&runtime_, {&transition_FB});
  EXPECT_THAT(states_to_exit, ElementsAre(&state_F, &state_C, &state_A));

  // External transition from E to D.
  MockTransition transition_ED(&state_E, &state_D, {"event_ED"});
  EXPECT_CALL(runtime_, GetActiveStates())
      .WillOnce(Return(StateSet{&state_E, &state_B}));
  states_to_exit = model_->ComputeExitSet(&runtime_, {&transition_ED});
  // The least common compound ancestor (LCCA) state_B is not exited.
  EXPECT_THAT(states_to_exit, ElementsAre(&state_E));

  // External transition from E to A.
  MockTransition transition_EA(&state_E, &state_A, {"event_EA"});
  EXPECT_CALL(runtime_, GetActiveStates())
      .WillOnce(Return(StateSet{&state_E, &state_B}));
  states_to_exit = model_->ComputeExitSet(&runtime_, {&transition_EA});
  EXPECT_THAT(states_to_exit, ElementsAre(&state_E, &state_B));

  // Internal transition from C to F. This is the special case where the target,
  // F, is a descendant of the source state C. The internal transition will not
  // exit and re-enter the source state C, but an external transition will.
  MockTransition transition_CF_internal(&state_C, {&state_F}, {"event_CF"}, "",
                                        true);
  EXPECT_CALL(runtime_, GetActiveStates())
      .WillOnce(Return(StateSet{&state_F, &state_C, &state_A}));
  states_to_exit = model_->ComputeExitSet(&runtime_, {&transition_CF_internal});
  EXPECT_THAT(states_to_exit, ElementsAre(&state_F));

  // External transition from C to F. C should be exited and re-entered.
  // Note that C's initial transition should not be taken.
  MockTransition transition_CF(&state_C, &state_F, {"event_CF"});
  EXPECT_CALL(runtime_, GetActiveStates())
      .WillOnce(Return(StateSet{&state_F, &state_C, &state_A}));
  states_to_exit = model_->ComputeExitSet(&runtime_, {&transition_CF});
  EXPECT_THAT(states_to_exit, ElementsAre(&state_F, &state_C));
}

TEST_F(ModelImplTest, ParallelCompoundIsInFinalState) {
  // Model document order: A B E C F D G
  //      A       <- Parallel state
  // ____|||____
  // |    |    |
  // B    C    D  <- Compound states
  // |    |    |
  // E    F    G  <- Final states
  //
  // Suppose E, F, G are active states.
  // All compound and parallel states are in final states.

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

  Reset({&state_a});

  ON_CALL(runtime_, GetActiveStates())
      .WillByDefault(Return(StateSet{&state_e, &state_f, &state_g}));

  // Atomic states are false.
  EXPECT_FALSE(model_->IsInFinalState(&runtime_, &state_e));
  EXPECT_FALSE(model_->IsInFinalState(&runtime_, &state_f));
  EXPECT_FALSE(model_->IsInFinalState(&runtime_, &state_g));

  // Compound states.
  EXPECT_TRUE(model_->IsInFinalState(&runtime_, &state_b));
  EXPECT_TRUE(model_->IsInFinalState(&runtime_, &state_c));
  EXPECT_TRUE(model_->IsInFinalState(&runtime_, &state_d));

  // Parallel state.
  EXPECT_TRUE(model_->IsInFinalState(&runtime_, &state_a));
}

TEST_F(ModelImplTest,
       ComputeEntrySetTransitToParallelDescendantEntersEntireBlock) {
  // Model document order: X A B E C F D G
  // X       A       <- Parallel state A
  //    ____|||____
  //    |    |    |
  //    B    C    D  <- Compound states
  //    |    |    |
  //    E    F    G  <- Final states
  //
  // X is an active state. Transition from X to F.

  MockState state_x("X");
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

  // Initial transitions.
  MockTransition transition_a_bcd(&state_a, {&state_b, &state_c, &state_d}, {});
  EXPECT_TRUE(state_a.SetInitialTransition(&transition_a_bcd));

  MockTransition transition_be(&state_b, &state_e);
  EXPECT_TRUE(state_b.SetInitialTransition(&transition_be));
  MockTransition transition_cf(&state_c, &state_f);
  EXPECT_TRUE(state_c.SetInitialTransition(&transition_cf));
  MockTransition transition_dg(&state_d, &state_g);
  EXPECT_TRUE(state_d.SetInitialTransition(&transition_dg));

  Reset({&state_x, &state_a});

  // Triggered transition to parallel descendant.
  MockTransition transition_xf(&state_x, &state_f);

  StateVec states_to_enter;
  StateSet states_for_default_entry;
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&transition_xf},
                                      &states_to_enter,
                                      &states_for_default_entry));

  // Entire parallel block is entered.
  EXPECT_THAT(states_to_enter,
              ElementsAre(&state_a, &state_b, &state_e, &state_c, &state_f,
                          &state_d, &state_g));
  // State C is not in default entry set as it is an ancestor of target state F.
  EXPECT_THAT(states_for_default_entry,
              UnorderedElementsAre(&state_b, &state_d));

  // Triggered transition to parallel top level.
  MockTransition transition_xa(&state_x, &state_a);

  states_to_enter.clear();
  states_for_default_entry.clear();
  EXPECT_TRUE(model_->ComputeEntrySet(&runtime_, {&transition_xa},
                                      &states_to_enter,
                                      &states_for_default_entry));

  // Entire parallel block is entered.
  EXPECT_THAT(states_to_enter,
              ElementsAre(&state_a, &state_b, &state_e, &state_c, &state_f,
                          &state_d, &state_g));
  // All compound state children of A are default entered.
  EXPECT_THAT(states_for_default_entry,
              UnorderedElementsAre(&state_b, &state_c, &state_d));
}

TEST_F(ModelImplTest, RemoveConflictingTransitionsNoConflictDoesNotRemove) {
  // Model document order: A B E C F D G
  //         A       <- Parallel state A
  //    ____|||____
  //    |    |    |
  //    B    C    D  <- Compound states
  //    |    |    |
  //    E    F    G  <- Atomic states

  MockState state_a("A", false, true);

  MockState state_b("B", false, false);
  MockState state_c("C", false, false);
  MockState state_d("D", false, false);

  MockState state_e("E");
  MockState state_f("F");
  MockState state_g("G");

  state_a.AddChildren({&state_b, &state_c, &state_d});
  state_b.AddChild(&state_e);
  state_c.AddChild(&state_f);
  state_d.AddChild(&state_g);

  // Non-conflicting self-transitions.
  MockTransition transition_e(&state_e, &state_e);
  MockTransition transition_f(&state_f, &state_f);
  MockTransition transition_g(&state_g, &state_g);

  Reset({&state_a});

  // All states active.
  ON_CALL(runtime_, GetActiveStates())
      .WillByDefault(Return(StateSet{&state_a, &state_b, &state_d, &state_c,
                                     &state_e, &state_f, &state_g}));

  Transitions filtered_transitions = model_->RemoveConflictingTransitions(
      &runtime_, {&transition_e, &transition_f, &transition_g});

  EXPECT_THAT(filtered_transitions,
              ElementsAre(&transition_e, &transition_f, &transition_g));
}

TEST_F(ModelImplTest, RemoveConflictingTransitionsWithConflictRemoves) {
  // Model document order: X Y A B E C F D G
  // X Y       A       <- Parallel state A
  //      ____|||____
  //      |    |    |
  //      B    C    D  <- Compound states
  //      |    |    |
  //      E    F    G  <- Atomic states

  MockState state_x("X");
  MockState state_y("Y");

  MockState state_a("A", false, true);

  MockState state_b("B", false, false);
  MockState state_c("C", false, false);
  MockState state_d("D", false, false);

  MockState state_e("E");
  MockState state_f("F");
  MockState state_g("G");

  state_a.AddChildren({&state_b, &state_c, &state_d});
  state_b.AddChild(&state_e);
  state_c.AddChild(&state_f);
  state_d.AddChild(&state_g);

  // Conflicting external transitions.
  MockTransition transition_ex(&state_e, &state_x);
  MockTransition transition_fy(&state_f, &state_y);
  MockTransition transition_gx(&state_g, &state_x);

  Reset({&state_x, &state_y, &state_a});

  // All states active.
  ON_CALL(runtime_, GetActiveStates())
      .WillByDefault(Return(StateSet{&state_a, &state_b, &state_d, &state_c,
                                     &state_e, &state_f, &state_g}));

  Transitions filtered_transitions = model_->RemoveConflictingTransitions(
      &runtime_, {&transition_ex, &transition_fy, &transition_gx});

  // By document order.
  EXPECT_THAT(filtered_transitions, ElementsAre(&transition_ex));
}

TEST_F(ModelImplTest, RemoveConflictingTransitionsDescendantPreemptsAncestor) {
  // Model document order: X Y A B E C F D G
  // X Y       A       <- Parallel state A
  //      ____|||____
  //      |    |    |
  //      B    C    D  <- Compound states
  //      |    |    |
  //      E    F    G  <- Atomic states

  MockState state_x("X");
  MockState state_y("Y");

  MockState state_a("A", false, true);

  MockState state_b("B", false, false);
  MockState state_c("C", false, false);
  MockState state_d("D", false, false);

  MockState state_e("E");
  MockState state_f("F");
  MockState state_g("G");

  state_a.AddChildren({&state_b, &state_c, &state_d});
  state_b.AddChild(&state_e);
  state_c.AddChild(&state_f);
  state_d.AddChild(&state_g);

  // Conflicting external transitions.
  MockTransition transition_ay(&state_a, &state_y);
  MockTransition transition_ex(&state_e, &state_x);

  Reset({&state_x, &state_y, &state_a});

  // All states active.
  ON_CALL(runtime_, GetActiveStates())
      .WillByDefault(Return(StateSet{&state_a, &state_b, &state_d, &state_c,
                                     &state_e, &state_f, &state_g}));

  Transitions filtered_transitions = model_->RemoveConflictingTransitions(
      &runtime_, {&transition_ay, &transition_ex});

  // Descendant source preempts ancestor source.
  EXPECT_THAT(filtered_transitions, ElementsAre(&transition_ex));
}

TEST_F(ModelImplTest, GetActiveStates) {
  // Model document order: X Y A B E C F D G
  // X Y       A
  //      ____|||____
  //      |    |    |
  //      B    C    D
  //      |    |    |
  //      E    F    G

  MockState state_x("X");
  MockState state_y("Y");

  MockState state_a("A", false, true);

  MockState state_b("B", false, false);
  MockState state_c("C", false, false);
  MockState state_d("D", false, false);

  MockState state_e("E");
  MockState state_f("F");
  MockState state_g("G");

  state_a.AddChildren({&state_b, &state_c, &state_d});
  state_b.AddChild(&state_e);
  state_c.AddChild(&state_f);
  state_d.AddChild(&state_g);

  Reset({&state_x, &state_y, &state_a});

  {  // Some state not found in the model is active. Return empty list.
    StateMachineContext::Runtime runtime =
        ParseTextOrDie<StateMachineContext::Runtime>(R"(
          active_state { id : "Z" })");
    EXPECT_THAT(model_->GetActiveStates(runtime.active_state()), ElementsAre());
  }
  {  // If we are to remain true to the StateChart standard, such a
     // configuration of active states is not possible. But since all states are
     // mocks anyway we use this configuration for testing.
    // TODO(srgandhe): Think of better format for serializing Runtime.
    StateMachineContext::Runtime runtime =
        ParseTextOrDie<StateMachineContext::Runtime>(R"(
            active_state { id : "X" }
            active_state {
              id : "A"
              active_child {
                id : "B"
                active_child { id : "E" }
              }
              active_child { id : "C" }
            })");
    EXPECT_THAT(model_->GetActiveStates(runtime.active_state()),
                ElementsAre(&state_x, &state_a, &state_b, &state_c, &state_e));
  }
}

}  // namespace
}  // namespace state_chart
