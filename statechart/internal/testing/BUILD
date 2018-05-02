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
#   A set of utilities needed for testing StateChart model.

package(default_visibility = [
    "//statechart:__subpackages__",
])

cc_library(
    name = "delegating_mock_executor",
    testonly = 1,
    hdrs = ["delegating_mock_executor.h"],
    deps = [
        "//statechart/internal:executor",
        "@com_google_googletest//:gtest",
    ],
)

proto_library(
    name = "json_value_coder_test_proto",
    srcs = ["json_value_coder_test.proto"],
    #    deps = ["//net/proto2/util/proto:json_format_proto"],
)

cc_proto_library(
    name = "json_value_coder_test_cc_proto",
    deps = ["json_value_coder_test_proto"],
)

cc_library(
    name = "mock_executable_content",
    testonly = 1,
    hdrs = ["mock_executable_content.h"],
    deps = [
        "//statechart/internal/model:executable_content",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "mock_event_dispatcher",
    testonly = 1,
    hdrs = ["mock_event_dispatcher.h"],
    deps = [
        "//statechart/internal:event_dispatcher",
        "//statechart/platform:types",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "mock_function_dispatcher",
    testonly = 1,
    hdrs = ["mock_function_dispatcher.h"],
    deps = [
        "//statechart/internal:function_dispatcher",
        "//statechart/platform:types",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "mock_datamodel",
    testonly = 1,
    hdrs = ["mock_datamodel.h"],
    deps = [
        "//statechart:logging",
        "//statechart/internal:datamodel",
        "//statechart/internal:runtime",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "mock_model",
    testonly = 1,
    hdrs = ["mock_model.h"],
    deps = [
        ":mock_transition",
        "//statechart/internal:model",
        "//statechart/platform:types",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "mock_runtime",
    testonly = 1,
    hdrs = ["mock_runtime.h"],
    deps = [
        ":mock_datamodel",
        ":mock_event_dispatcher",
        "//statechart/internal:datamodel",
        "//statechart/internal:event_dispatcher",
        "//statechart/internal:runtime",
        "//statechart/proto:state_machine_context_cc_proto",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "mock_state",
    testonly = 1,
    hdrs = ["mock_state.h"],
    deps = [
        "//statechart/internal/model:state",
        "//statechart/platform:types",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "mock_transition",
    testonly = 1,
    hdrs = ["mock_transition.h"],
    deps = [
        "//statechart/internal/model:transition",
        "//statechart/platform:types",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "state_chart_builder",
    testonly = 1,
    hdrs = ["state_chart_builder.h"],
    deps = [
        "//statechart/platform:protobuf",
        "//statechart/platform:types",
        "//statechart/proto:state_chart_cc_proto",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "state_chart_builder_test",
    srcs = ["state_chart_builder_test.cc"],
    deps = [
        ":state_chart_builder",
        "//statechart/platform:protobuf",
        "//statechart/platform:test_util",
        "//statechart/platform:types",
        "//statechart/proto:state_chart_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
