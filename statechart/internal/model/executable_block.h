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

#ifndef STATE_CHART_INTERNAL_MODEL_EXECUTABLE_BLOCK_H_
#define STATE_CHART_INTERNAL_MODEL_EXECUTABLE_BLOCK_H_

#include <ostream>
#include <vector>

#include <glog/logging.h>

#include "statechart/internal/model/executable_content.h"

namespace state_chart {
class Runtime;
}

namespace state_chart {
namespace model {

// A class that encapsulates a sequence of executable content.
class ExecutableBlock : public ExecutableContent {
 public:
  // Skips over items in 'executables' which are nullptr.
  explicit ExecutableBlock(
      const std::vector<const ExecutableContent*>& executables) {
    for (auto executable : executables) {
      if (executable != nullptr) {
        executables_.push_back(executable);
      } else {
        LOG(DFATAL) << "Not adding null executable block to executable content"
                    << std::endl;
      }
    }
  }

  ExecutableBlock(const ExecutableBlock&) = delete;
  ExecutableBlock& operator=(const ExecutableBlock&) = delete;

  ~ExecutableBlock() override = default;

  // Executes all executables in sequence order. Stop execution of the block if
  // any executable raises an error (returns false).
  bool Execute(Runtime* runtime) const override {
    for (auto executable : executables_) {
      if (!executable->Execute(runtime)) {
        return false;
      }
    }
    return true;
  }

 private:
  std::vector<const ExecutableContent*> executables_;
};

}  // namespace model
}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_MODEL_EXECUTABLE_BLOCK_H_
