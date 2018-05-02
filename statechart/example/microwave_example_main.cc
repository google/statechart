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

#include <glog/logging.h>
#include <gflags/gflags.h>

#include "absl/strings/substitute.h"
#include "statechart/internal/function_dispatcher_impl.h"
#include "statechart/platform/protobuf.h"
#include "statechart/proto/state_chart.pb.h"
#include "statechart/proto/state_machine_context.pb.h"
#include "statechart/state_machine_factory.h"
#include "statechart/state_machine.h"
#include "statechart/example/microwave.pb.h"

namespace {

using ::proto2::contrib::parse_proto::ParseTextOrDie;
using ::state_chart::config::StateChart;
using ::state_chart::StateMachineFactory;
using ::state_chart::StateMachine;
using ::state_chart::StateMachineContext;
using ::state_chart::FunctionDispatcherImpl;
using ::example::microwave::MicrowaveState;
using ::example::microwave::MicrowavePayload;

constexpr const char kMicrowaveStateChart[] = R"PROTO(
  name: "microwave"
  datamodel: {
    data: {
      id: "state"
      expr: "{ \"light\" : \"OFF\" }"
    }
  }
  state: {
    # Parallel state will keep all its children states active at the same time.
    # We use the parallel construct to separate orthogonal concerns in the 
    # system being represented, such as state of door, light and power.
    parallel: {
      state: {
        # Create a state that is responsible for monitor whether the door is 
        # open or not.
        state: {
          id: "door"
          initial_id: "door_is_closed"
          state: {
            state: {
              id: "door_is_open"
              onentry: {
                log: { label: "DoorState" expr: "'Door is Open.'" }
              }
              transition: {
                # Whenever the user closes the door, the environment will fire
                # an event named 'event.CloseDoor'.
                event: "event.CloseDoor"
                target: "door_is_closed"
              }
            }
          }
          state: {
            state: {
              id: "door_is_closed"
              onentry: {
                log: { label: "DoorState" expr: "'Door is Closed.'" }
              }
              transition: {
                event: "event.OpenDoor"
                target: "door_is_open"
              }
            }
          }
        }
      }
      state: {
        # Create a state that is responsible for turning on/off the light.
        state: {
          id: "light_controller"
          initial_id: "light_off"
          state: {
            state: {
              id: "light_off"
              transition: {
                # The light turns on if there is power and if the door is open
                # or the oven is cooking.
                # NOTE: This is an eventless transition.
                cond: "In('power_on') && (In('door_is_open') || In('cooking'))"
                target: "light_on"
                executable: {
                  assign: { location: "state.light" expr: "'ON'" }
                }
              }
            }
          }
          state: {
            state: {
              id: "light_on"
              transition: {
                cond: "!(In('power_on') && (In('door_is_open') || In('cooking')))"
                target: "light_off"
                executable: {
                  # Similar logically equivalent executable code could've also
                  # been specified as onentry in 'light_on', 'light_off' states.
                  assign: { location: "state.light" expr: "'OFF'" }
                }
              }
            }
          }
        }
      }
      state: {
        # Create a state that is responsible for keeping track of whether the
        # device is powered on or not.
        state: {
          id: "oven"
          initial_id: "power_off"
          state: {
            state: {
              id: "power_on"
              # If initial_id is missing, the first child state is used as
              # initial (i.e., 'idle').
              onentry: {
                assign: { location: "state.cooking_duration_sec" expr: "0" }
              }
              transition: {
                event: "event.PowerOff"
                target: "power_off"
              }
              state {
                state {
                  id: "idle"
                  transition: {
                    event: "event.StartCooking"
                    target: "cooking"
                    executable: {
                      # The payload is available as '_event.data'.
                      # log statements can be used for printf debugging :).
                      log: { label: "Payload" expr: "_event" }
                    }
                    executable: {
                      assign: { 
                        location: "state.cooking_duration_sec"
                        # NOTE: During protobuf to JSON transform the field name
                        # 'duration_sec' changes to 'durationSec'.
                        expr: "_event.data.durationSec"
                      }
                    }
                  }
                  transition: {
                    event: "event.Resume"
                    target: "cooking"
                  }
                }
              }
              state {
                state {
                  id: "cooking"
                  transition: {
                    event: "event.Pause"
                    target: "idle"
                  }
                  transition: {
                    event: "event.TimeTick"
                    target: "cooking"
                    executable: {
                      assign: { 
                        location: "state.cooking_duration_sec"
                        # NOTE: This is ridiculous and can be easily replaced 
                        # by 'state.cooking_duration_sec - 1'. But it's here
                        # to show that arbitrary registered functions can be 
                        # called.
                        # expr: "state.cooking_duration_sec - 1"
                        expr: "Decrement(state.cooking_duration_sec)"
                      }
                    }
                  }
                  transition: {
                    # If the cooking duration is zero or less, move to idle.
                    cond: "state.cooking_duration_sec <= 0"
                    target: "idle"
                  }
                  transition: {
                    # If the door is opened move to idle state. 
                    cond: "In('door_is_open')"
                    target: "idle"
                  }
                }
              }
            }
          }
          state: {
            state: {
              id: "power_off"
              transition: {
                event: "event.PowerOn"
                target: "power_on"
              }
            }
          }
        }
      }
    }
  }
)PROTO";

StateChart GetMicrowaveStateChart() {
  return ParseTextOrDie<StateChart>(kMicrowaveStateChart);
}

void PrintCookingDurationAndLight(const StateMachine& state_machine) {
  MicrowaveState state;
  CHECK(state_machine.ExtractMessageFromDatamodel("state", &state));
  LOG(INFO) << ::absl::Substitute("Cooking duration $0, Light is $1",
                                  state.cooking_duration_sec(),
                                  MicrowaveState::Light_Name(state.light()));
}

int Decrement(int i) {
  return i - 1;
}

}  // namespace



int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  const auto microwave_sc = GetMicrowaveStateChart();
  LOG(INFO) << microwave_sc.DebugString();

  // Create a Factory that knows about StateChart configuration.
  std::unique_ptr<StateMachineFactory> sc_factory =
      StateMachineFactory::CreateFromProtos(
          std::vector<StateChart>{microwave_sc});

  // Create a default FunctionDispather.
  FunctionDispatcherImpl function_dispatcher;
  CHECK(function_dispatcher.RegisterFunction("Decrement", Decrement));

  std::unique_ptr<StateMachine> microwave =
      sc_factory->CreateStateMachine("microwave", &function_dispatcher);

  LOG(INFO) << "---------------------- Initializing ...";
  microwave->Start();
  LOG(INFO) << "---------------------- Initialization complete.";

  PrintCookingDurationAndLight(*microwave);

  // Open the door of the microwave.
  LOG(INFO) << "---------------------- User opens the door.";
  microwave->SendEvent("event.OpenDoor", "");
  // light should be turned off still, since there is no power.
  PrintCookingDurationAndLight(*microwave);

  // Let's turn on power.
  LOG(INFO) << "---------------------- User turns on the power.";
  microwave->SendEvent("event.PowerOn", "");
  // The light should turn on now since the door is open.
  PrintCookingDurationAndLight(*microwave);

  // Put in some popcorn.
  // Close the door of the microwave.
  LOG(INFO) << "---------------------- User closes the door.";
  microwave->SendEvent("event.CloseDoor", "");
  // light should be turned off.
  PrintCookingDurationAndLight(*microwave);

  // Let's cook for 10 seconds.
  LOG(INFO) << "---------------------- User presses start with duration 10s.";
  const auto start_payload =
      ParseTextOrDie<MicrowavePayload>("duration_sec : 10");
  microwave->SendEvent("event.StartCooking", &start_payload);
  PrintCookingDurationAndLight(*microwave);


  LOG(INFO) << "---------------------- Time Starts ticking...";
  for(int i = 0; i < 7; ++i) {
    microwave->SendEvent("event.TimeTick", "");
    PrintCookingDurationAndLight(*microwave);
  }

  // Open the door of the microwave while it was running.
  LOG(INFO) << "---------------------- User opens the door.";
  microwave->SendEvent("event.OpenDoor", "");
  PrintCookingDurationAndLight(*microwave);

  LOG(INFO) << "---------------------- User closes the door.";
  microwave->SendEvent("event.CloseDoor", "");
  PrintCookingDurationAndLight(*microwave);

  for (int i = 7; i < 15; ++i) {
    microwave->SendEvent("event.TimeTick", "");
    PrintCookingDurationAndLight(*microwave);
  }

  LOG(INFO) << "---------------------- User pressed the resume button.";
  microwave->SendEvent("event.Resume", "");
  PrintCookingDurationAndLight(*microwave);

  for (int i = 15; i < 20; ++i) {
    microwave->SendEvent("event.TimeTick", "");
    PrintCookingDurationAndLight(*microwave);
  }

  LOG(INFO) << "---------------------- Times up!.";
  PrintCookingDurationAndLight(*microwave);

  LOG(INFO) << "---------------------- User presses start with duration 10s.";
  const auto start_payload_new =
      ParseTextOrDie<MicrowavePayload>("duration_sec : 10");
  microwave->SendEvent("event.StartCooking", &start_payload_new);
  PrintCookingDurationAndLight(*microwave);
  LOG(INFO) << "---------------------- Time Starts ticking...";
  for (int i = 0; i < 6; ++i) {
    microwave->SendEvent("event.TimeTick", "");
    PrintCookingDurationAndLight(*microwave);
  }

  LOG(INFO) << "---------------------- Serialize/De-serialize";
  // Serialize the internal state of the 'microwave' state machine.
  StateMachineContext state_machine_context;
  CHECK(microwave->SerializeToContext(&state_machine_context));
  // Destroy the StateMachine instance.
  microwave.reset();

  // Reconstruct a new instance based on previous context.
  auto microwave_new = sc_factory->CreateStateMachine(
      "microwave", state_machine_context, &function_dispatcher);
  for (int i = 6; i < 12; ++i) {
    microwave_new->SendEvent("event.TimeTick", "");
    PrintCookingDurationAndLight(*microwave_new);
  }

  return 0;
}
