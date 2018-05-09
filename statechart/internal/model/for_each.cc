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

#include "statechart/internal/model/for_each.h"

#include <glog/logging.h>

#include "absl/strings/str_cat.h"
#include "absl/strings/substitute.h"
#include "statechart/internal/datamodel.h"
#include "statechart/internal/model/executable_content.h"
#include "statechart/internal/runtime.h"
#include "statechart/internal/utility.h"

namespace state_chart {
namespace model {

ForEach::ForEach(const string& array, const string& item, const string& index,
                 const ExecutableContent* body)
    : array_(array), item_(item), index_(index), body_(body) {}

// override
bool ForEach::Execute(Runtime* runtime) const {
  VLOG(1) << absl::Substitute("ForEach(<$0, $1> : $2)", index_, item_, array_);
  auto* datamodel = runtime->mutable_datamodel();
  // Get the iterator.
  auto iterator = datamodel->EvaluateIterator(array_);
  if (iterator == nullptr) {
    runtime->EnqueueExecutionError(absl::StrCat(
        "'ForEach' unable to get iterator for collection: ", array_));
    return false;
  }
  // Create item variable if needed.
  if (!datamodel->IsDefined(item_) && !datamodel->Declare(item_)) {
    runtime->EnqueueExecutionError(
        absl::StrCat("'ForEach' unable to declare item variable at: ", item_));
    return false;
  }
  // Create the index variable if specified.
  if (!index_.empty() && !datamodel->IsDefined(index_) &&
      !datamodel->Declare(index_)) {
    runtime->EnqueueExecutionError(absl::StrCat(
        "'ForEach' unable to declare index variable at: ", index_));
    return false;
  }

  // Execute the loop.
  string value;
  string index;
  for (; !iterator->AtEnd(); iterator->Next()) {
    value = iterator->GetValue();
    // Assign the item.
    if (!datamodel->AssignExpression(item_, value)) {
      runtime->EnqueueExecutionError(
          absl::StrCat("'ForEach' unable to assign item variable '", item_,
                       "' with value: ", value));
      return false;
    }
    // Assign the index if needed.
    if (!index_.empty()) {
      index = iterator->GetIndex();
      if (!datamodel->AssignExpression(index_, index)) {
        runtime->EnqueueExecutionError(
            absl::StrCat("'ForEach' unable to assign index variable '", index_,
                         "' with value: ", index));
        return false;
      }
    }
    // Run the loop body, empty executable blocks are nullptr.
    if (body_ != nullptr && !body_->Execute(runtime)) {
      return false;
    }
  }
  return true;
}

}  // namespace model
}  // namespace state_chart
