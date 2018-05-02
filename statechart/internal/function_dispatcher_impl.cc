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

#include "statechart/internal/function_dispatcher_impl.h"

#include <glog/logging.h>

#include "statechart/internal/function_dispatcher_builtin.h"
#include "statechart/platform/map_util.h"
#include "absl/memory/memory.h"

namespace state_chart {

namespace internal {

std::vector<string> JsonValuesToStrings(
    const std::vector<const Json::Value*>& values) {
  std::vector<string> json_strs;
  for (const auto* json_value : values) {
    json_strs.push_back(Json::FastWriter().write(*json_value));
  }
  return json_strs;
}

}  // namespace internal

FunctionDispatcherImpl::FunctionDispatcherImpl() {
  RegisterFunction("ContainsKey", &builtin::ContainsKey);
  RegisterFunction("FindFirstWithKeyValue", &builtin::FindFirstWithKeyValue);
}

FunctionDispatcherImpl::FunctionDispatcherImpl(
    const FunctionDispatcherImpl& other) {
  for (const auto& entry : other.function_map_) {
    function_map_.emplace(entry.first,
                          ::absl::WrapUnique(entry.second->Clone()));
  }
}

// override
bool FunctionDispatcherImpl::HasFunction(const string& function_name) const {
  return gtl::ContainsKey(function_map_, function_name);
}

// override
bool FunctionDispatcherImpl::Execute(
    const string& function_name, const std::vector<const Json::Value*>& inputs,
    Json::Value* return_value) {
  if (!HasFunction(function_name)) {
    LOG(INFO) << "No function registered for name : " << function_name;
    return false;
  }
  return function_map_.at(function_name)->Execute(inputs, return_value);
}

}  // namespace state_chart
