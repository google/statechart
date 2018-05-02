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

#include "statechart/internal/model/log.h"

#include <glog/logging.h>

#include "absl/strings/str_cat.h"
#include "statechart/internal/datamodel.h"
#include "statechart/internal/runtime.h"
#include "statechart/internal/utility.h"

namespace state_chart {
namespace model {

Log::Log(const string& label, const string& expr)
    : label_(label), expr_(expr) {}

// override
bool Log::Execute(Runtime* runtime) const {
  VLOG(1) << "Log: " << label_ << ": " << expr_;
  string log_string;
  if (!runtime->datamodel().EvaluateStringExpression(expr_, &log_string)) {
    runtime->EnqueueExecutionError(
        absl::StrCat("'Log' expression failed to evaluate to string: ", expr_));
    return false;
  }
  LOG(INFO) << (label_.empty() ? "" : label_ + ": ") << log_string;
  return true;
}

}  // namespace model
}  // namespace state_chart
