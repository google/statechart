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

#include "statechart/json_utils.h"

#include <gtest/gtest.h>

#include "statechart/proto/state_machine_context.pb.h"


namespace state_chart {
namespace json_utils {
namespace {

const char kExpectedDebugString[] = R"(datamodel: #-- JSON --# {
   "baz" : {
      "nested" : "inside"
   },
   "foo" : "bar",
   "hello" : "world"
} #-- JSON --#
)";

TEST(JsonUtils, DebugStringTest) {
  ::state_chart::StateMachineContext state_machine_context;
  state_machine_context.set_datamodel(R"(
      { "hello" : "world", "foo" : "bar", "baz" : { "nested" : "inside" } })");
  EXPECT_EQ(kExpectedDebugString,
            json_utils::DebugString(state_machine_context));
}

TEST(JsonUtils, DebugStringTestNotJson) {
  ::state_chart::StateMachineContext state_machine_context;
  state_machine_context.set_datamodel("Not \"really\" JSON");
  EXPECT_EQ("datamodel: \"Not \\\"really\\\" JSON\"\n",
            json_utils::DebugString(state_machine_context));
}

}  // namespace
}  // namespace json_utils
}  // namespace state_chart
