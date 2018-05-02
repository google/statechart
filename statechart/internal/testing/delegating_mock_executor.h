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

#ifndef STATE_CHART_INTERNAL_TESTING_DELEGATING_MOCK_EXECUTOR_H_
#define STATE_CHART_INTERNAL_TESTING_DELEGATING_MOCK_EXECUTOR_H_

#include <gmock/gmock.h>

#include "statechart/internal/executor.h"

namespace state_chart {

// A testing class that employs "delegate to parent" so that calls from one
// Executor method to other Executor methods may be mocked when needed.
// By default all calls will be delegated to actual Executor methods.
class DelegatingMockExecutor : public Executor {
 public:
  DelegatingMockExecutor() {
    using ::testing::_;
    ON_CALL(*this, Start(_, _)).WillByDefault(
        testing::Invoke(this, &DelegatingMockExecutor::RealStart));
    ON_CALL(*this, SendEvent(_, _, _, _)).WillByDefault(
        testing::Invoke(this, &DelegatingMockExecutor::RealSendEvent));
    ON_CALL(*this, ExecuteUntilStable(_, _)).WillByDefault(
        testing::Invoke(this, &DelegatingMockExecutor::RealExecuteUntilStable));
    ON_CALL(*this, ProcessExternalEvent(_, _, _, _))
        .WillByDefault(testing::Invoke(
            this, &DelegatingMockExecutor::RealProcessExternalEvent));
    ON_CALL(*this, MicroStep(_, _, _)).WillByDefault(
        testing::Invoke(this, &DelegatingMockExecutor::RealMicroStep));
    ON_CALL(*this, EnterStates(_, _, _)).WillByDefault(
        testing::Invoke(this, &DelegatingMockExecutor::RealEnterStates));
    ON_CALL(*this, ExitStates(_, _, _)).WillByDefault(
        testing::Invoke(this, &DelegatingMockExecutor::RealExitStates));
    ON_CALL(*this, AssignEventData(_, _, _)).WillByDefault(
        testing::Invoke(this, &DelegatingMockExecutor::RealAssignEventData));
    ON_CALL(*this, Shutdown(_, _)).WillByDefault(
        testing::Invoke(this, &DelegatingMockExecutor::RealShutdown));
  }

  MOCK_CONST_METHOD2(Start, void(const Model* model, Runtime* runtime));

  void RealStart(const Model* model, Runtime* runtime) const {
    Executor::Start(model, runtime);
  }

  MOCK_CONST_METHOD4(SendEvent,
                     void(const Model* model, Runtime* runtime,
                          const string& event, const string& payload));

  void RealSendEvent(const Model* model, Runtime* runtime, const string& event,
                     const string& payload) const {
    Executor::SendEvent(model, runtime, event, payload);
  }

  MOCK_CONST_METHOD2(ExecuteUntilStable,
                     void(const Model* model, Runtime* runtime));

  void RealExecuteUntilStable(const Model* model, Runtime* runtime) const {
    Executor::ExecuteUntilStable(model, runtime);
  }

  MOCK_CONST_METHOD4(ProcessExternalEvent,
                     void(const Model* model, Runtime* runtime,
                          const string& event, const string& payload));

  void RealProcessExternalEvent(const Model* model, Runtime* runtime,
                                const string& event,
                                const string& payload) const {
    Executor::ProcessExternalEvent(model, runtime, event, payload);
  }

  MOCK_CONST_METHOD3(
      MicroStep,
      void(const Model* model, Runtime* runtime,
           const std::vector<const model::Transition*>& transitions));

  void RealMicroStep(
      const Model* model, Runtime* runtime,
      const std::vector<const model::Transition*>& transitions) const {
    Executor::MicroStep(model, runtime, transitions);
  }

  MOCK_CONST_METHOD3(
      EnterStates,
      void(const Model* model, Runtime* runtime,
           const std::vector<const model::Transition*>& transitions));

  void RealEnterStates(
      const Model* model, Runtime* runtime,
      const std::vector<const model::Transition*>& transitions) const {
    Executor::EnterStates(model, runtime, transitions);
  }

  MOCK_CONST_METHOD3(
      ExitStates,
      void(const Model* model, Runtime* runtime,
           const std::vector<const model::Transition*>& transitions));

  void RealExitStates(
      const Model* model, Runtime* runtime,
      const std::vector<const model::Transition*>& transitions) const {
    Executor::ExitStates(model, runtime, transitions);
  }

  MOCK_CONST_METHOD3(AssignEventData,
                     void(Runtime* runtime, const string& event,
                          const string& payload));

  void RealAssignEventData(Runtime* runtime, const string& event,
                           const string& payload) const {
    Executor::AssignEventData(runtime, event, payload);
  }

  MOCK_CONST_METHOD2(Shutdown, void(const Model* model, Runtime* runtime));

  void RealShutdown(const Model* model, Runtime* runtime) const {
    Executor::Shutdown(model, runtime);
  }
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_TESTING_DELEGATING_MOCK_EXECUTOR_H_
