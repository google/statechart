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

#ifndef STATE_CHART_STATE_MACHINE_H_
#define STATE_CHART_STATE_MACHINE_H_

#include <set>
#include <string>

#include "statechart/internal/datamodel.h"  // IWYU pragma: export
#include "statechart/internal/model.h"  // IWYU pragma: export
#include "statechart/internal/runtime.h"  // IWYU pragma: export
#include "statechart/platform/protobuf.h"
#include "statechart/platform/types.h"
#include "statechart/proto/state_machine_context.pb.h"

namespace state_chart {

class StateMachineListener;

// Abstract base class for a state machine in the state_chart framework.
class StateMachine {
 public:
  StateMachine();
  virtual ~StateMachine() {}

  // Starts execution of the StateMachine. This must be called before any calls
  // to SendEvent().
  virtual void Start() = 0;

  // Sends the external event given by 'event' to the state machine, with an
  // optional 'payload' string. An empty 'payload' indicates no payload. The
  // format of the payload depends on the Datamodel configured for the state
  // machine. The default is ECMAScript, which accepts JSON-formatted payloads.
  //
  // Control is returned to the caller only once 'event' has been consumed, and
  // any state transitions that result from it have been executed.
  virtual void SendEvent(const string& event, const string& payload) = 0;

  // Convenience method for passing a proto buffer as a payload. If non-NULL,
  // 'payload' is converted to a JSON string and then passed into the State
  // Machine. With the ECMAScript Datamodel (the default), the JSON object will
  // be accessible with a structure equivalent to the proto buffer.
  virtual void SendEvent(const string& event,
                         const proto2::Message* payload);

  // Adds 'listener' as a listener on this state machine. Takes ownership of
  // 'listener'.
  virtual void AddListener(StateMachineListener* listener) = 0;

  // Returns a read-only reference to the Runtime object for this instance. This
  // allows examination of active states, and the state machine model.
  virtual const Runtime& GetRuntime() const = 0;

  // Returns a reference to the Model underlying this StateMachine.
  virtual const Model& GetModel() const = 0;

  // Returns name of the model used by this state machine.
  string GetModelName() const { return GetModel().GetName(); }

  // Returns the JsonFormat object used to convert from protocol message to JSON
  // in the StateMachine datamodel. This is exposed to allow users to call
  // JsonFormat::SetFieldToJsonTagMapping() for specific FieldDescriptors,
  // allowing custom field name mappings in the JSON objects. This is
  // particularly useful for proto2 extensions, which have ugly and (depending
  // on the datamodel type used) inaccessible field names.
  //
  // Another (simpler) option for field name mapping can be used if you own the
  // .proto files in question: use proto2 field annotations and specify
  // (proto2.util.json_opt).name in the .proto file. See
  // net/proto2/util/proto/json_format.proto for examples.
  proto2::util::JsonFormat* mutable_json_format();

  const proto2::util::JsonFormat& json_format() const;

  // A convenience method to pull an object from the State Machine's datamodel
  // and store it in 'message_output'. The format of 'datamodel_location' is
  // specific to the type of datamodel in use. The default allows JSON
  // object-access notation (i.e., "myobject.field1.subfield2").
  //
  // Return false if nothing exists at 'datamodel_location', or if converting
  // from JSON to proto fails.
  bool ExtractMessageFromDatamodel(const string& datamodel_location,
                                   proto2::Message* message_output) const;

  // Serialize the current state_machine state into state_machine_context.
  // Returns true if successful.
  // Serializing a state machine which is not allowed to run to quiescence will
  // fail.
  bool SerializeToContext(StateMachineContext* state_machine_Context) const;

 private:
  // Formatter used to convert to and from JSON in the datamodel.
  proto2::util::JsonFormat json_format_;

  // A list of proto message names for which we have configured field mappings
  // in 'json_format_'.
  std::set<string> json_format_mapped_descriptors_;
};

}  // namespace state_chart

#endif  // STATE_CHART_STATE_MACHINE_H_
