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

#include <string>
#include <type_traits>

#include "statechart/internal/testing/state_chart_builder.h"
#include "statechart/platform/protobuf.h"
#include "statechart/platform/test_util.h"
#include "statechart/platform/types.h"
#include "statechart/proto/state_chart.pb.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace state_chart {
namespace config {
namespace {

using ::testing::ElementsAre;
using ::testing::EqualsProto;

TEST(StateChartBuilderTest, ExecutableBlockBuilderTest) {
  proto2::RepeatedPtrField<ExecutableElement> e;

  StateChart embedded_sc;
  embedded_sc.set_name("embedded");

  ExecutableBlockBuilder(&e)
      .AddRaise("event")
      .AddLog("label", "expr")
      .AddAssign("location", "expr")
      .AddScript("src", "content")
      .AddSend("event", "eventexpr", "target", "targetexpr", "type", "typeexpr",
               "id", "idlocation", "delay", "delayexpr", { "name1", "name2" })
        .AddParam("name", "expr", "location")
        .SetContentExpr("contentexpr")
        .SetContentString("contentstring")
        .SetContentStateChart(embedded_sc);

  EXPECT_THAT(e, ElementsAre(
      EqualsProto("raise { event: 'event' }"),
      EqualsProto("log { label: 'label' expr: 'expr' }"),
      EqualsProto("assign { location: 'location' expr: 'expr' }"),
      EqualsProto("script { src: 'src' content: 'content' }"),
      EqualsProto(R"BUF(
                    send {
                      event: 'event'
                      eventexpr: 'eventexpr'
                      target: 'target'
                      targetexpr: 'targetexpr'
                      type: 'type'
                      typeexpr: 'typeexpr'
                      id: 'id'
                      idlocation: 'idlocation'
                      delay: 'delay'
                      delayexpr: 'delayexpr'
                      namelist: 'name1'
                      namelist: 'name2'
                      param: { name: 'name' expr: 'expr' location: 'location' }
                      content {
                        expr: 'contentexpr'
                        content: 'contentstring'
                        state_chart {
                          name: 'embedded'
                        }
                      }
                    }
                  )BUF")
          ));
}

TEST(StateChartBuilderTest, ExecutableBlockBuilderTest_If) {
  // Basic If block with one level.
  proto2::RepeatedPtrField<ExecutableElement> e;

  ExecutableBlockBuilder(&e)
      .AddIf("condition")
        .AddRaise("event")
      .ElseIf("condition2")
        .AddRaise("event2")
      .Else()
        .AddRaise("event3")
      .EndIf();

  EXPECT_THAT(e, ElementsAre(
      EqualsProto(R"BUF(
                    if {
                      cond_executable {
                        cond: 'condition'
                        executable {
                          raise { event: 'event' }
                        }
                      }
                      cond_executable {
                        cond: 'condition2'
                        executable {
                          raise { event: 'event2' }
                        }
                      }
                      cond_executable {
                        executable {
                          raise { event: 'event3' }
                        }
                      }
                    }
                  )BUF")));

  // Nested If blocks.
  e.Clear();
  ExecutableBlockBuilder(&e)
      .AddIf("cond1")
        .AddIf("cond1_1")
          .AddRaise("event1_1")
        .EndIf()
        .AddRaise("event1")
      .Else()
        .AddIf("cond2_1")
          .AddRaise("event2_1")
        .EndIf()
      .EndIf();

  EXPECT_THAT(e, ElementsAre(
      EqualsProto(R"BUF(
                    if {
                      cond_executable {
                        cond: 'cond1'
                        executable {
                          if {
                            cond_executable {
                              cond: 'cond1_1'
                              executable {
                                raise { event: 'event1_1' }
                              }
                            }
                          }
                        }
                        executable {
                          raise { event: 'event1' }
                        }
                      }
                      cond_executable {
                        executable {
                          if {
                            cond_executable {
                              cond: 'cond2_1'
                              executable {
                                raise { event: 'event2_1' }
                              }
                            }
                          }
                        }
                      }
                    }
                  )BUF")));
}

TEST(StateChartBuilderTest, ExecutableBlockBuilderTest_ForEach) {
  proto2::RepeatedPtrField<ExecutableElement> e;

  ExecutableBlockBuilder(&e)
      .AddForEach("array", "item", "index")
        .AddRaise("hi")
      .EndForEach()
      .AddRaise("hello");

  EXPECT_THAT(e, ElementsAre(
      EqualsProto(R"BUF(
                    foreach {
                      array: 'array'
                      item: 'item'
                      index: 'index'
                      executable {
                        raise { event: 'hi' }
                      }
                    }
                  )BUF"),
      EqualsProto("raise { event: 'hello' }")));

  // Nested case.
  e.Clear();
  ExecutableBlockBuilder(&e)
      .AddForEach("array", "item", "index")
        .AddForEach("array2", "item2", "index2")
          .AddRaise("hi")
        .EndForEach()
      .EndForEach();

  EXPECT_THAT(e, ElementsAre(
      EqualsProto(R"BUF(
                    foreach {
                      array: 'array'
                      item: 'item'
                      index: 'index'
                      executable {
                        foreach {
                          array: 'array2'
                          item: 'item2'
                          index: 'index2'
                          executable {
                            raise { event: 'hi' }
                          }
                        }
                      }
                    }
                  )BUF")));
}

TEST(StateChartBuilderTest, ExecutableBlockBuilderTest_NestedIfForEach) {
  // Nest different types of actions: if and foreach.
  proto2::RepeatedPtrField<ExecutableElement> e;

  ExecutableBlockBuilder(&e)
      .AddIf("cond")
        .AddForEach("array", "item", "index")
          .AddRaise("hi")
        .EndForEach()
        .AddRaise("hello")
      .EndIf()
      .AddRaise("howdy");

  EXPECT_THAT(e, ElementsAre(
      EqualsProto(R"BUF(
                    if {
                      cond_executable {
                        cond: 'cond'
                        executable {
                          foreach {
                            array: 'array'
                            item: 'item'
                            index: 'index'
                            executable {
                              raise { event: 'hi' }
                            }
                          }
                        }
                        executable {
                          raise { event: 'hello' }
                        }
                      }
                    }
                  )BUF"),
      EqualsProto("raise { event: 'howdy' }")));
}

TEST(StateChartBuilderTest, DataModelBuilder) {
  DataModel d;

  DataModelBuilder(&d)
      .AddDataFromExpr("id", "expr")
      .AddDataFromSrc("id2", "src");

  EXPECT_THAT(d, EqualsProto(R"BUF(
      data { id: 'id' expr: 'expr' }
      data { id: 'id2' src: 'src' }
  )BUF"));
}

TEST(StateChartBuilderTest, BuildTransition) {
  Transition t;

  BuildTransition(&t, {"event1", "event2"},
                  {"target"}, "cond", Transition::TYPE_INTERNAL)
      .AddRaise("event");

  EXPECT_THAT(t, EqualsProto(R"BUF(
      event: 'event1'
      event: 'event2'
      cond: 'cond'
      target: 'target'
      type: TYPE_INTERNAL,
      executable {
        raise { event: 'event' }
      }
  )BUF"));
}

TEST(StateChartBuilderTest, StateBuilder) {
  State s;
  StateBuilder builder(&s, "id");

  builder.AddInitialId("initial");
  builder.DataModel()
      .AddDataFromExpr("id", "expr");
  builder.OnEntry()
      .AddRaise("onentry");
  builder.OnExit()
      .AddRaise("onexit");
  builder.SetInitialTransition("initial_target")
      .AddRaise("initial_transition");

  builder.AddTransition({"event1", "event2"}, {"target1"}, "cond",
                        Transition::TYPE_INTERNAL)
      .AddRaise("transition1");
  builder.AddTransition({"event3"}, {"target2"});

  EXPECT_THAT(s, EqualsProto(R"BUF(
      id: 'id'
      initial_id: 'initial'
      initial {
        transition {
          target: 'initial_target'
          executable {
            raise { event: 'initial_transition' }
          }
        }
      }
      datamodel {
        data { id: 'id' expr: 'expr' }
      }
      onentry { raise { event: 'onentry' } }
      onexit { raise: { event: 'onexit' } }
      transition {
        event: 'event1'
        event: 'event2'
        cond: 'cond'
        target: 'target1'
        type: TYPE_INTERNAL
        executable {
          raise { event: 'transition1' }
        }
      }
      transition {
        event: 'event3'
        target: 'target2'
      }
  )BUF"));
}

TEST(StateChartBuilderTest, CompoundStateBuilder) {
  State s;
  StateBuilder builder(&s, "A");
  builder.AddState("B").AddState("C");
  builder.AddState("D");

  EXPECT_THAT(s, EqualsProto(R"BUF(
    id: 'A'
    state: { state: { id: 'B' state: { state: { id: 'C' } } } }
    state: { state: { id: 'D' } }
   )BUF"));
}

TEST(StateChartBuilderTest, ParallelBuilder) {
  Parallel s;
  ParallelBuilder builder(&s, "id");

  builder.DataModel()
      .AddDataFromExpr("id", "expr");
  builder.OnEntry()
      .AddRaise("onentry");
  builder.OnExit()
      .AddRaise("onexit");

  builder.AddTransition({"event1", "event2"}, {"target1"}, "cond",
                        Transition::TYPE_INTERNAL)
      .AddRaise("transition1");
  builder.AddTransition({"event3"}, {"target2"});

  EXPECT_THAT(s, EqualsProto(R"BUF(
      id: 'id'
      datamodel {
        data { id: 'id' expr: 'expr' }
      }
      onentry { raise { event: 'onentry' } }
      onexit { raise: { event: 'onexit' } }
      transition {
        event: 'event1'
        event: 'event2'
        cond: 'cond'
        target: 'target1'
        type: TYPE_INTERNAL
        executable {
          raise { event: 'transition1' }
        }
      }
      transition {
        event: 'event3'
        target: 'target2'
      }
  )BUF"));
}

TEST(StateChartBuilderTest, FinalStateBuilder) {
  Final s;
  FinalStateBuilder builder(&s, "id");

  builder.OnEntry()
      .AddRaise("onentry");
  builder.OnExit()
      .AddRaise("onexit");

  EXPECT_THAT(s, EqualsProto(R"BUF(
      id: 'id'
      onentry { raise { event: 'onentry' } }
      onexit { raise: { event: 'onexit' } }
  )BUF"));
}

TEST(StateChartBuilderTest, StateChartBuilder) {
  StateChart sc;
  StateChartBuilder builder(&sc, "test");

  builder
      .AddInitial("initial")
      .SetDataModelType("foo")
      .SetBinding(StateChart::BINDING_LATE);

  builder.AddState("state")
      .AddInitialId("state_initial_id");

  builder.AddFinalState("final");

  builder.DataModel()
      .AddDataFromExpr("id", "data");

  EXPECT_THAT(sc, EqualsProto(R"BUF(
      initial: 'initial'
      name: 'test'
      datamodel_type: 'foo'
      binding: BINDING_LATE
      datamodel {
        data { id: 'id' expr: 'data' }
      }
      state {
        state {
          id: 'state'
          initial_id: 'state_initial_id'
        }
      }
      state {
        final {
          id: 'final'
        }
      }
  )BUF"));
}

}  // namespace
}  // namespace config
}  // namespace state_chart
