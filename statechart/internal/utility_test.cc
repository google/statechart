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

#include "statechart/internal/utility.h"

#include <functional>
#include <map>
#include <string>

#include <glog/logging.h>
#include <gtest/gtest.h>

namespace state_chart {
namespace {

TEST(UtilityTest, MakeJSONFromStringMapTest) {
  // Test empty case.
  EXPECT_EQ("{}", MakeJSONFromStringMap({}));
  // Test normal case with quote escaping.
  EXPECT_EQ(
      R"JSON({"key1":1,"key2":{},"key3":"unquoted","key4":"\"quoted\""})JSON",
      MakeJSONFromStringMap({{"key1", "1"},
                             {"key2", "{}"},
                             {"key3", R"("unquoted")"},
                             {"key4", R"("\"quoted\"")"}}));
}

TEST(UtilityTest, MaybeJSON) {
  // Ensure that JSON with newlines inside of them can be picked up as JSON.
  EXPECT_TRUE(MaybeJSON("{}"));
  EXPECT_TRUE(MaybeJSON("\n\n{\n\n\n}\n\n"));
}

TEST(UtilityTest, SearchWithPredicates) {
  std::vector<int> binary{0, 1, 0, 0, 1, 1, 1, 0};
  auto expected = binary.begin() + 2;
  auto actual = SearchWithPredicates(
      binary.begin(), binary.end(),
      std::vector<std::function<bool(int)>>{[](int x) { return x == 0; },
                                       [](int x) { return x == 0; }});
  LOG(INFO) << "search expected: " << expected - binary.begin();
  LOG(INFO) << "search actual: " << actual - binary.begin();
  EXPECT_EQ(expected, actual);

  expected = binary.begin() + 5;
  actual = SearchWithPredicates(
      binary.begin(), binary.end(),
      std::vector<std::function<bool(int)>>{
          [](int x) { return x == 1; },
          [](int x) { return x == 1; },
          [](int x) { return x == 0; },
      });
  LOG(INFO) << "search expected: " << expected - binary.begin();
  LOG(INFO) << "search actual: " << actual - binary.begin();
  EXPECT_EQ(expected, actual);
}

TEST(UtilityTest, FindOuterMostParentheses) {
  struct TestCase {
    string sequence;
    int start;
    int end;
  };

  TestCase cases[] = {
      // Fail cases.
      {"", 0, 0},
      {"a", 1, 1},
      {"(", 1, 1},
      {"(()", 3, 3},
      // Pass cases.
      {"(abc)", 0, 4},
      {"(())", 0, 3},
      {"()()", 0, 1},
      {"(()())", 0, 5},
      {"())", 0, 1},
  };

  for (const auto& test : cases) {
    auto result = FindOuterMostParentheses(
        test.sequence.begin(), test.sequence.end(),
        [](char x) { return x == '('; }, [](char x) { return x == ')'; });
    LOG(INFO) << "FindParenthesis test: " << test.sequence;
    EXPECT_EQ(test.start, result.first - test.sequence.begin());
    EXPECT_EQ(test.end, result.second - test.sequence.begin());
  }
}

}  // namespace
}  // namespace state_chart
