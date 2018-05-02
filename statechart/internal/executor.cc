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

#include "statechart/internal/executor.h"

#include <set>
#include <utility>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "absl/algorithm/container.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/substitute.h"
#include "statechart/internal/datamodel.h"
#include "statechart/internal/event_dispatcher.h"
#include "statechart/internal/model.h"
#include "statechart/internal/model/model.h"
#include "statechart/internal/runtime.h"
#include "statechart/internal/utility.h"
#include "statechart/logging.h"
#include "statechart/proto/state_chart.pb.h"

DEFINE_int32(max_num_microsteps, 1000,
             "The maximum number of microsteps to run inside one macrostep.");

namespace state_chart {

namespace {

using ::absl::c_all_of;

// Execute some content, does nothing if nullptr is passed to it.
void Execute(Runtime* runtime, const model::ExecutableContent* executable) {
  if (executable != nullptr) {
    executable->Execute(runtime);
  }
}

// Walks through the model in document order and initializes all datamodels
// by executing them.
void InitializeDatamodel(Runtime* runtime,
                         const model::ExecutableContent* datamodel,
                         const std::vector<const model::State*>& states) {
  Execute(runtime, datamodel);
  for (auto state : states) {
    InitializeDatamodel(runtime, state->GetDatamodelBlock(),
                        state->GetChildren());
  }
}

// Declare a datamodel variable or enqueue an error.
bool DeclareOrEnqueueError(Runtime* runtime, const string& id) {
  if (!runtime->mutable_datamodel()->Declare(id)) {
    runtime->EnqueueExecutionError(absl::StrCat("Declare failed: ", id));
    return false;
  }
  return true;
}

// Assign a datamodel string to variable or enqueue an error.
// Returns true if assign was successful.
bool AssignStringOrEnqueueError(Runtime* runtime, const string& id,
                                const string& str) {
  if (!runtime->mutable_datamodel()->AssignString(id, str)) {
    runtime->EnqueueExecutionError(
        absl::StrCat("AssignString failed: ", id, " = ", str));
    return false;
  }
  return true;
}

// Assign a datamodel expression to variable or enqueue an error.
// Returns true if assign was successful.
bool AssignExpressionOrEnqueueError(Runtime* runtime, const string& id,
                                    const string& expr) {
  if (!runtime->mutable_datamodel()->AssignExpression(id, expr)) {
    runtime->EnqueueExecutionError(
        absl::StrCat("AssignExpression failed: ", id, " = ", expr));
    return false;
  }
  return true;
}

// Return true if an event name is an error event, i.e., it starts with "error"
// or "error.".
bool IsErrorEvent(const string& event) {
  return event == "error" || absl::StartsWith(event, "error.");
}

}  // namespace

// virtual
void Executor::Start(const Model* model, Runtime* runtime) const {
  RETURN_IF(model == nullptr || runtime == nullptr);
  RETURN_IF_MSG(runtime->IsRunning(), "No op; runtime is already running.");

  runtime->Clear();
  runtime->SetRunning(true);

  // Bind system variables according to specifications.
  DeclareOrEnqueueError(runtime, "_name");
  AssignStringOrEnqueueError(runtime, "_name", model->GetName());
  // Use runtime address as unique session id.
  DeclareOrEnqueueError(runtime, "_sessionid");
  AssignStringOrEnqueueError(runtime, "_sessionid",
                             absl::Substitute("SESSION_$0", runtime));
  // Create the event object.
  DeclareOrEnqueueError(runtime, "_event");
  AssignExpressionOrEnqueueError(runtime, "_event",
                                 runtime->datamodel().EncodeParameters({}));

  // There is currently no late binding support.
  if (model->GetDatamodelBinding() == config::StateChart::BINDING_EARLY) {
    // The SCXML specifications does not seem to dictate an early binding
    // datamodel initialization order. We use document order.
    InitializeDatamodel(runtime, model->GetDatamodelBlock(),
                        model->GetTopLevelStates());
  } else {
    // TODO(qplau): Late binding support initialization.
    LOG(DFATAL) << "StateChart::BINDING_LATE is not supported.";
  }

  // TODO(qplau): Script support, execute top level script.

  // Start with the top level transition of the model.
  EnterStates(model, runtime, {model->GetInitialTransition()});

  // Run till external events are required or complete.
  ExecuteUntilStable(model, runtime);
}

// virtual
void Executor::SendEvent(const Model* model, Runtime* runtime,
                         const string& event, const string& payload) const {
  RETURN_IF(model == nullptr || runtime == nullptr);
  RETURN_IF(!runtime->IsRunning());

  ProcessExternalEvent(model, runtime, event, payload);
  ExecuteUntilStable(model, runtime);
}

// virtual
void Executor::ExecuteUntilStable(const Model* model, Runtime* runtime) const {
  std::vector<const model::Transition*> transitions;

  // The loop execution is one macrostep.
  for (int num_microsteps = 0;
       runtime->IsRunning() && num_microsteps < FLAGS_max_num_microsteps;
       ++num_microsteps) {
    transitions = model->GetEventlessTransitions(runtime);
    if (transitions.empty()) {
      if (!runtime->HasInternalEvent()) {
        break;
      } else {
        std::pair<string, string> event_payload =
            runtime->DequeueInternalEvent();

        // Raises error event if assignment fails.
        AssignEventData(runtime, event_payload.first, event_payload.second);

        transitions =
            model->GetTransitionsForEvent(runtime, event_payload.first);

        // Terminate the macro step if an error event is unhandled.
        // This is not in the SCXML specification but we use it to prevent
        // infinite loops from executing. One way infinite loop occurs is when
        // all eventless transitions have conditions resulting in
        // 'error.execution' internal events being enqueued.
        if (IsErrorEvent(event_payload.first)) {
          if (transitions.empty()) {
            LOG(INFO) << absl::Substitute(
                "[ERROR] Macro step prematurely terminated due to unhandled "
                "error (event: $0, payload: $1). Runtime:\n$2",
                event_payload.first, event_payload.second,
                runtime->DebugString());
            break;  // Stop the macro step.
          } else {
            LOG(INFO) << "[ERROR] event: " << event_payload.first
                      << ", payload: " << event_payload.second;
          }
        }
      }
    }

    if (!transitions.empty()) {
      MicroStep(model, runtime, transitions);
    }
  }

  if (!runtime->IsRunning()) {
    Shutdown(model, runtime);
    return;
  }

  // TODO(qplau): Add code to Invoke. This must end with recursive call to
  // ExecuteUntilStable if Invoke raises internal events.
  // Invoke should also be skipped if state machine has reached final state.
  // That is,
  //
  //  < Invoke code goes here >
  //  if (runtime->HasInternalEvent()) {
  //    ExecuteUntilStable(model, runtime);
  //  }
}

// virtual
void Executor::ProcessExternalEvent(const Model* model, Runtime* runtime,
                                    const string& event,
                                    const string& payload) const {
  // Raise an execution error if the datamodel cannot assign '_event'.
  AssignEventData(runtime, event, payload);

  // Select transitions that can run.
  // This is the selectTransitions() procedure.
  const std::vector<const model::Transition*> transitions =
      model->GetTransitionsForEvent(runtime, event);

  // TODO(qplau): Implement isCancelEvent logic.
  // TODO(qplau): Implement Invoke finalization and forwarding.

  if (!transitions.empty()) {
    MicroStep(model, runtime, transitions);
  }
}

// virtual
void Executor::MicroStep(
    const Model* model, Runtime* runtime,
    const std::vector<const model::Transition*>& transitions) const {
  ExitStates(model, runtime, transitions);
  for (auto transition : transitions) {
    Execute(runtime, transition->GetExecutable());
    runtime->GetEventDispatcher()->NotifyTransitionFollowed(runtime,
                                                            transition);
  }
  EnterStates(model, runtime, transitions);
}

// Pseudo-code for EnterStates().
//
// procedure enterStates(enabledTransitions):
//  statesToEnter = new OrderedSet()
//  statesForDefaultEntry = new OrderedSet()
//  computeEntrySet(enabledTransitions, statesToEnter, statesForDefaultEntry)
//  for s in statesToEnter.toList().sort(entryOrder):
//    configuration.add(s)
//    statesToInvoke.add(s)
//    if binding == "late" and s.isFirstEntry:
//      initializeDataModel(datamodel.s,doc.s)
//      s.isFirstEntry = false
//    for content in s.onentry:
//      executeContent(content)
//    if statesForDefaultEntry.isMember(s):
//      executeContent(s.initial.transition)
//    if isFinalState(s):
//      if isSCXMLElement(s.parent):
//        running = false
//      else:
//        parent = s.parent
//        grandparent = parent.parent
//        internalQueue.enqueue(
//            new Event("done.state." + parent.id, s.donedata))
//        if isParallelState(grandparent):
//          if getChildStates(grandparent).every(isInFinalState):
//            internalQueue.enqueue(new Event("done.state." + grandparent.id))
//
// virtual
void Executor::EnterStates(
    const Model* model, Runtime* runtime,
    const std::vector<const model::Transition*>& transitions) const {
  std::vector<const model::State*> states_to_enter;
  // The set compound states that will be entered due to some descendant state
  // being entered.
  std::set<const model::State*> states_for_default_entry;
  // TODO(ufirst): we need to add check that ComputeEntrySet did not fail,
  //  by checking the return value. Note that this will require changing this
  //  function and many others to return bool.
  model->ComputeEntrySet(runtime, transitions, &states_to_enter,
                         &states_for_default_entry);

  for (const auto* state : states_to_enter) {
    runtime->AddActiveState(state);

    // TODO(qplau):
    // 1. Add to invoke list.
    // 2. Late binding: Initialize datamodel, requires 'IsFirstEntry' tracking
    //    in runtime.

    Execute(runtime, state->GetOnEntry());

    runtime->GetEventDispatcher()->NotifyStateEntered(runtime, state);

    if (states_for_default_entry.find(state) !=
        states_for_default_entry.end()) {
      // If the state exists in states_for_default_entry, it must not be an
      // initial state. Hence it must have an initial transition.
      if (state->GetInitialTransition() != nullptr) {
        Execute(runtime, state->GetInitialTransition()->GetExecutable());
      } else {
        LOG(DFATAL) << "State '" << state->id()
                    << "' should have specified an initial transition.";
      }
    }

    if (state->IsFinal()) {
      // If the Final state is a top level final state.
      if (state->GetParent() == nullptr) {
        runtime->SetRunning(false);
      } else {
        // TODO(qplau): Prepare the done data.
        // Create internal done event.
        runtime->EnqueueInternalEvent(
            absl::StrCat("done.state.", state->GetParent()->id()), "");

        // Handle parallel state logic.
        // If grand parent is parallel, check to see if grand parent's children
        // are all final. Note that IsInFinalState() is recursive, hence the
        // nested parallel states case is handled, albeit with duplicate calls
        // to IsInFinalState() on the same state/parallel element as
        // 'states_to_enter' is in document order.
        auto grand_parent = state->GetParent()->GetParent();
        if (grand_parent != nullptr && grand_parent->IsParallel()) {
          if (c_all_of(grand_parent->GetChildren(),
                       [model, runtime](const model::State* s) {
                         return model->IsInFinalState(runtime, s);
                       })) {
            runtime->EnqueueInternalEvent(
                absl::StrCat("done.state.", grand_parent->id()), "");
          }
        }
      }
    }
  }
}

// Psuedo-code for ExitStates():
//
// procedure exitStates(enabledTransitions):
//   statesToExit = computeExitSet(enabledTransitions)
//   for s in statesToExit:
//       statesToInvoke.delete(s)
//   statesToExit = statesToExit.toList().sort(exitOrder)
//   for s in statesToExit:
//       for h in s.history:
//           if h.type == "deep":
//               f = lambda s0: isAtomicState(s0) and isDescendant(s0,s)
//           else:
//               f = lambda s0: s0.parent == s
//            historyValue[h.id] = configuration.toList().filter(f)
//   for s in statesToExit:
//       for content in s.onexit:
//           executeContent(content)
//       for inv in s.invoke:
//           cancelInvoke(inv)
//       configuration.delete(s)
//
// virtual
void Executor::ExitStates(
    const Model* model, Runtime* runtime,
    const std::vector<const model::Transition*>& transitions) const {
  // States are sorted in exit order.
  const auto states = model->ComputeExitSet(runtime, transitions);

  // TODO(qplau): Remove states from "states to invoke" queue.
  // TODO(qplau): Implement history processing.

  for (const auto* state : states) {
    Execute(runtime, state->GetOnExit());
    // TODO(qplau): Cancel invokes in the state.
    runtime->EraseActiveState(state);
    runtime->GetEventDispatcher()->NotifyStateExited(runtime, state);
  }
}

// TODO(qplau): Add assigning other event fields to support add an event type,
// <send>, and <invoke>.
//
// virtual
void Executor::AssignEventData(Runtime* runtime, const string& event,
                               const string& payload) const {
  // Fail early if one field assignment fails since only one error should be
  // raised.
  if (!AssignStringOrEnqueueError(runtime, "_event.name", event)) {
    return;
  }
  if (!payload.empty()) {
    AssignExpressionOrEnqueueError(runtime, "_event.data", payload);
  }
}

// SCXML pseudo-code:
// procedure exitInterpreter():
//  statesToExit = configuration.toList().sort(exitOrder)
//  for s in statesToExit:
//    for content in s.onexit:
//        executeContent(content)
//    for inv in s.invoke:
//        cancelInvoke(inv)
//    configuration.delete(s)
//    if isFinalState(s) and isScxmlState(s.parent):
//        returnDoneEvent(s.donedata)
//
// virtual
void Executor::Shutdown(const Model* model, Runtime* runtime) const {
  const auto active_state_set = runtime->GetActiveStates();
  std::vector<const model::State*> active_states(active_state_set.begin(),
                                            active_state_set.end());
  model->SortStatesByDocumentOrder(true, &active_states);

  for (const auto& state : active_states) {
    Execute(runtime, state->GetOnExit());
    // TODO(qplau): Cancel invoke.
    runtime->EraseActiveState(state);
    // TODO(qplau): Return donedata.
  }

  // Dequeue remaining internal events to look for errors to report.
  // This behavior is not specified in the state chart specifications.
  std::pair<string, string> event_payload;
  while (runtime->HasInternalEvent()) {
    event_payload = runtime->DequeueInternalEvent();
    LOG_IF(WARNING, IsErrorEvent(event_payload.first))
        << "[ERROR] event: " << event_payload.first
        << ", payload: " << event_payload.second;
  }
}

}  // namespace state_chart
