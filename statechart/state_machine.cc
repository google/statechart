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

#include "statechart/state_machine.h"

#include <vector>

#include <glog/logging.h>

#include "statechart/platform/protobuf.h"
#include "statechart/logging.h"

using proto2::util::JsonFormat;

namespace state_chart {

StateMachine::StateMachine()
    : json_format_(JsonFormat::ADD_WHITESPACE |
                   JsonFormat::QUOTE_LARGE_INTS |
                   JsonFormat::USE_JSON_OPT_PARAMETERS |
                   JsonFormat::SYMBOLIC_ENUMS) {}

void StateMachine::SendEvent(const string& event,
                             const proto2::Message* payload) {
  string json_payload;
  if (payload != nullptr) {
    mutable_json_format()->PrintToString(*payload, &json_payload);
  }
  SendEvent(event, json_payload);
}

proto2::util::JsonFormat* StateMachine::mutable_json_format() {
  return &json_format_;
}

const proto2::util::JsonFormat& StateMachine::json_format() const {
  return json_format_;
}

bool StateMachine::ExtractMessageFromDatamodel(
    const string& datamodel_location, proto2::Message* message_output) const {
  string json_object;
  const auto& datamodel = GetRuntime().datamodel();
  if (!datamodel.IsDefined(datamodel_location) ||
      !datamodel.EvaluateExpression(datamodel_location, &json_object)) {
    return false;
  }

  return json_format().ParseFromString(json_object, message_output);
}

bool StateMachine::SerializeToContext(
    StateMachineContext* state_machine_context) const {
  const auto& runtime = GetRuntime();
  if (runtime.HasInternalEvent()) {
    return false;
  }
  *state_machine_context->mutable_runtime() = runtime.Serialize();
  state_machine_context->set_datamodel(runtime.datamodel().SerializeAsString());
  return true;
}

}  // namespace state_chart
