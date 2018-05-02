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

#include "statechart/internal/model.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace state_chart {
namespace {

struct EventTestCase {
  std::vector<string> events;
  string event_name;
  bool result;
};

TEST(Model, EventsMatchTest) {
  const EventTestCase kTestCases[] = {
      // Single matching.
      {{"A"}, "A", true},
      {{"B"}, "A", false},
      // Multiple matching.
      {{"A", "B"}, "A", true},
      {{"A", "B"}, "B", true},
      {{"A", "B", "D"}, "C", false},
      // Hierarchical matching.
      {{"A"}, "A.A1", true},
      {{"A.A1"}, "A.A1.AA1", true},
      {{"A.A1"}, "A.A2", false},
      // Multiple hierarchies.
      {{"A", "B", "C"}, "A.A1.AA1", true},
      {{"A", "B", "C"}, "C.C1.CC1", true},
      {{"A", "B", "C"}, "E.E1.EE1", false},
      {{"A.A1", "B", "C.C1.CC1"}, "A.A1.AA1", true},
      {{"A.A1", "B", "C.C1.CC1"}, "C.C1.CC2", false},
      {{"A.A1", "B", "C.C1.CC1"}, "B.B1.BB1", true},
      // Star matches everything.
      {{"*"}, "A", true},
      {{"B", "*"}, "A", true},
      {{"A.A1", "*", "C.C1.CC1"}, "C.C1.CC2", true},
  };

  for (const auto& test_case : kTestCases) {
    EXPECT_EQ(test_case.result,
        Model::EventMatches(test_case.event_name, test_case.events));
  }
}

}  // namespace
}  // namespace state_chart
