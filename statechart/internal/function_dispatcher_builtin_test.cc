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

#include "statechart/internal/function_dispatcher_builtin.h"

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <gtest/gtest.h>

#include "statechart/platform/types.h"
#include "include/json/json.h"

namespace state_chart {
namespace builtin {
namespace {

TEST(FunctionDispatcherBuiltins, ContainsKey) {
  Json::Reader reader;
  Json::Value value;
  CHECK(reader.parse(R"({ "K3" : { "lower" : "l" } } )", value));
  EXPECT_FALSE(ContainsKey(value, "K"));
  EXPECT_TRUE(ContainsKey(value, "K3"));
  // Specifying a path as key does not work.
  EXPECT_FALSE(ContainsKey(value, "K3.lower"));
}

TEST(FunctionDispatcherBuiltins, FindFirstWithKeyValue) {
  Json::Reader reader;
  Json::Value value;
  CHECK(reader.parse(R"([ { "K1" : "V1" } ,
                          { "K2" : "V2", "foo" : "bar" },
                          { "K2" : "V2"},
                          { "K3" : { "lower" : "l" }} 
                        ])",
                     value));

  EXPECT_EQ(-1, FindFirstWithKeyValue(value, "K", "V1"));
  EXPECT_EQ(-1, FindFirstWithKeyValue(value, "K1", ""));
  EXPECT_EQ(0, FindFirstWithKeyValue(value, "K1", "V1"));
  EXPECT_EQ(1, FindFirstWithKeyValue(value, "K2", "V2"));

  Json::Value lower_value;
  CHECK(reader.parse(R"({ "lower" : "l" })", lower_value));
  EXPECT_EQ(3, FindFirstWithKeyValue(value, "K3", lower_value));

  // Specifying a path as key does not work.
  EXPECT_EQ(-1, FindFirstWithKeyValue(value, "K3.lower", "l"));
}

}  // namespace
}  // namespace builtin
}  // namespace state_chart
