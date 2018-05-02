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

#include "include/json/json.h"

namespace state_chart {
namespace builtin {

bool ContainsKey(const Json::Value& value, const string& field_name) {
  return value.isMember(field_name);
}

int FindFirstWithKeyValue(const Json::Value& array, const string& key,
                          const Json::Value& value) {
  if (array.isArray()) {
    for (unsigned int i = 0; i < array.size(); ++i) {
      if (array[i].isMember(key) && (array[i][key] == value)) {
        return i;
      }
    }
  }
  return -1;
}

}  // namespace builtin
}  // namespace state_chart
