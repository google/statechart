# Copyright 2018 The StateChart Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Description:
#   A package for testing utilities for StateChart library.

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "mock_state_machine",
    testonly = 1,
    hdrs = ["mock_state_machine.h"],
    deps = [
        "//statechart:state_machine",
        "//statechart/internal/testing:mock_model",
        "//statechart/internal/testing:mock_runtime",
        "//statechart/platform:protobuf",
    ],
)

proto_library(
    name = "state_machine_test_proto",
    srcs = ["state_machine_test.proto"],
)

cc_proto_library(
    name = "state_machine_test_cc_proto",
    deps = [":state_machine_test_proto"],
)
