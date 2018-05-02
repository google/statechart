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

#ifndef STATE_CHART_INTERNAL_TESTING_MOCK_STATE_H_
#define STATE_CHART_INTERNAL_TESTING_MOCK_STATE_H_

#include <gmock/gmock.h>
#include <string>
#include <vector>

#include "statechart/platform/types.h"
#include "statechart/internal/model/state.h"

namespace state_chart {

// Convenience typedef of gmock Return() actions for StateInterface.
typedef std::set<const model::State*> StateSet;
typedef std::vector<const model::State*> StateVec;

class MockState : public model::State {
 public:
  explicit MockState(const string& id, bool is_final = false,
                     bool is_parallel = false,
                     const model::ExecutableContent* datamodel = nullptr,
                     const model::ExecutableContent* on_entry = nullptr,
                     const model::ExecutableContent* on_exit = nullptr)
      : model::State(id, is_final, is_parallel, datamodel, on_entry, on_exit) {}
  MockState(const MockState&) = delete;
  MockState& operator=(const MockState&) = delete;
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_TESTING_MOCK_STATE_H_
