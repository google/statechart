/*
 * Copyright 2018 The StateChart Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// A set of utility function for manipulating datamodel objects.
// These are builtin into every instance of FunctionDispatcher.
#ifndef STATE_CHART_INTERNAL_FUNCTION_DISPATCHER_BUILTIN_H_
#define STATE_CHART_INTERNAL_FUNCTION_DISPATCHER_BUILTIN_H_

#include <string>

#include "statechart/platform/types.h"

namespace Json { class Value; }

namespace state_chart {
namespace builtin {

// Checks whether a particular field_name exists in a Json value.
bool ContainsKey(const Json::Value& value, const string& field_name);

// Finds the index of the first object in a 'array' of objects where the
// object has a specified <key, value> pair.
// Returns -1 if no such matching object is found.
// Returns -1 if the input array is not a Json array.
// E.g.,
// if array = [ { "K1" : "V1" } ,
//              { "K2" : "V2", "foo" : "bar" },
//              { "K2" : "V2"} ] then,
// EXPECT_EQ(0, FindFirstWithKeyValue(array, "K1", "V1"))
// EXPECT_EQ(-1, FindFirstWithKeyValue(array, "K1", ""))
// EXPECT_EQ(1, FindFirstWithKeyValue(array, "K2", "V2"))
int FindFirstWithKeyValue(const Json::Value& array, const string& key,
                          const Json::Value& value);

}  // namespace builtin
}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_FUNCTION_DISPATCHER_BUILTIN_H_
