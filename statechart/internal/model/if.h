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

// IWYU pragma: private, include "src/internal/model/model.h"
// IWYU pragma: friend src/internal/model/*

#ifndef STATE_CHART_INTERNAL_MODEL_IF_H_
#define STATE_CHART_INTERNAL_MODEL_IF_H_

#include <string>
#include <vector>

#include "statechart/platform/types.h"
#include "statechart/internal/model/executable_content.h"

namespace state_chart {
class Runtime;
}  // namespace state_chart

namespace state_chart {
namespace model {

class If : public ExecutableContent {
 public:
  // Accepts a list of pairs of condition, executable. The logic for If will
  // loop through the configuration, evaluate the condition and execute the
  // first executable for which the condition evaluates to true.
  explicit If(const std::vector<std::pair<string, const ExecutableContent*>>&
                  condition_executable);
  If(const If&) = delete;
  If& operator=(const If&) = delete;

  bool Execute(Runtime* runtime) const override;

 private:
  const std::vector<std::pair<string, const ExecutableContent*>>
      condition_executable_;
};

}  // namespace model
}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_MODEL_IF_H_
