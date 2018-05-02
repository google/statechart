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

#ifndef STATE_CHART_INTERNAL_MODEL_FOR_EACH_H_
#define STATE_CHART_INTERNAL_MODEL_FOR_EACH_H_

#include "statechart/platform/types.h"
#include "statechart/internal/model/executable_content.h"

namespace state_chart {
class Runtime;
}

namespace state_chart {
namespace model {

// Encapsulates execution logic for a 'foreach' element in state charts.
class ForEach : public ExecutableContent {
 public:
  // Params:
  //   array Location expression of the iterable collection (array).
  //   item  Location expression of the item (value).
  //   index Location expression of the index, may be empty to ignore.
  //   body  The body of the foreach, usually an ExecutableBlock. Does not take
  //         ownership.
  ForEach(const string& array, const string& item, const string& index,
          const ExecutableContent* body);

  ForEach(const ForEach&) = delete;
  ForEach& operator=(const ForEach&) = delete;


  ~ForEach() override = default;

  // If an error occurs while running the body of the foreach, the loop is
  // terminated and false returned.
  bool Execute(Runtime* runtime) const override;

 private:
  const string array_;
  const string item_;
  const string index_;
  const ExecutableContent* const body_;
};

}  // namespace model
}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_MODEL_FOR_EACH_H_
