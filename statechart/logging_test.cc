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

#include "statechart/logging.h"

#include <gtest/gtest.h>

namespace state_chart {
namespace {

static const int kMagicNum = 42;

int* ReturnIntPtr(int x) {
  RETURN_NULL_IF(x != kMagicNum);
  return new int(kMagicNum);
}

bool ReturnBool(int x) {
  RETURN_FALSE_IF(x != kMagicNum);
  return true;
}

TEST(ReturnNullIfTest, ReturnsNonNull) {
  std::unique_ptr<int> ptr(ReturnIntPtr(kMagicNum));
  EXPECT_NE(nullptr, ptr);
}

TEST(ReturnNullIfTest, ReturnsNull) {
  EXPECT_DEBUG_DEATH(
      { EXPECT_EQ(nullptr, ReturnIntPtr(20)); },
      "Returning nullptr; condition \\(x != kMagicNum\\) is true.");
}

TEST(ReturnFalseIfTest, ReturnsTrue) { EXPECT_TRUE(ReturnBool(kMagicNum)); }

TEST(ReturnFalseIfTest, ReturnsFalse) {
  EXPECT_DEBUG_DEATH(
      { EXPECT_FALSE(ReturnBool(20)); },
      "Returning false; condition \\(x != kMagicNum\\) is true.");
}

}  // namespace
}  // namespace state_chart
