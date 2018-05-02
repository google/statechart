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

#ifndef STATE_CHART_INTERNAL_FUNCTION_DISPATCHER_H_
#define STATE_CHART_INTERNAL_FUNCTION_DISPATCHER_H_

#include <string>
#include <vector>

#include "statechart/platform/types.h"

namespace Json { class Value; }

namespace state_chart {

// This base class executes C++ functions with a given function name and
// arguments.
class FunctionDispatcher {
 public:
  FunctionDispatcher() = default;
  FunctionDispatcher(const FunctionDispatcher&) = delete;
  FunctionDispatcher& operator=(const FunctionDispatcher&) = delete;
  virtual ~FunctionDispatcher() = default;

  // Returns whether a function identified by the 'function_name' is registered
  // with this function dispatcher.
  virtual bool HasFunction(const string& function_name) const = 0;

  // Executes a function identified by the 'function_name' using the 'inputs'
  // arguments and sets the output to '*return_value'.
  // Returns true on successful function execution.
  virtual bool Execute(const string& function_name,
                       const std::vector<const Json::Value*>& inputs,
                       Json::Value* return_value) = 0;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_FUNCTION_DISPATCHER_H_
