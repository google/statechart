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
#   Libraries for a generic StateChart executor based on SCXML standard.

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "state_machine",
    srcs = ["state_machine.cc"],
    hdrs = ["state_machine.h"],
    deps = [
        ":logging",
        "//statechart/internal:datamodel",
        "//statechart/internal:model",
        "//statechart/internal:runtime",
        "//statechart/platform:protobuf",
        "//statechart/proto:state_machine_context_cc_proto",
    ],
)
# TODO(srgandhe): Fix this test.
#cc_test(
#    name = "state_machine_test",
#    size = "small",
#    srcs = ["state_machine_test.cc"],
#    deps = [
#        ":state_machine",
#        "//statechart/internal/testing:mock_datamodel",
#        "//statechart/internal/testing:mock_runtime",
#        "//statechart/platform:test_util",
#        "//statechart/proto:state_machine_context_cc_proto",
#        "//statechart/testing:mock_state_machine",
#        "//statechart/testing:state_machine_test_cc_proto",
#        "@com_google_googletest//:gtest_main",
#    ],
#)

cc_library(
    name = "state_machine_factory",
    srcs = ["state_machine_factory.cc"],
    hdrs = ["state_machine_factory.h"],
    deps = [
        ":logging",
        ":state_machine",
        ":state_machine_listener",
        "//statechart/internal:datamodel",
        "//statechart/internal:executor",
        "//statechart/internal:function_dispatcher",
        "//statechart/internal:light_weight_datamodel",
        "//statechart/internal:model",
        "//statechart/internal:model_builder",
        "//statechart/internal:model_impl",
        "//statechart/internal:runtime",
        "//statechart/internal:runtime_impl",
        "//statechart/internal:state_machine_impl",
        "//statechart/internal:state_machine_logger",
        "//statechart/platform:map_util",
        "//statechart/proto:state_chart_cc_proto",
        "//statechart/proto:state_machine_context_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "state_machine_factory_test",
    srcs = ["state_machine_factory_test.cc"],
    deps = [
        ":state_machine",
        ":state_machine_factory",
        "//statechart/internal:function_dispatcher",
        "//statechart/internal:model",
        "//statechart/internal:runtime",
        "//statechart/internal/model",
        "//statechart/internal/testing:mock_function_dispatcher",
        "//statechart/internal/testing:state_chart_builder",
        "//statechart/platform:test_util",
        "//statechart/proto:state_chart_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "state_machine_listener",
    hdrs = ["state_machine_listener.h"],
    deps = ["//statechart/platform:types"],
)

cc_library(
    name = "logging",
    hdrs = ["logging.h"],
    deps = [
        "//statechart/platform:logging",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "logging_test",
    srcs = ["logging_test.cc"],
    deps = [
        ":logging",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "json_utils",
    srcs = ["json_utils.cc"],
    hdrs = ["json_utils.h"],
    deps = [
        "//statechart/platform:map_util",
        "//statechart/platform:protobuf",
        "//statechart/platform:types",
        "//statechart/proto:state_machine_context_cc_proto",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@jsoncpp_git//:jsoncpp",
    ],
)

cc_test(
    name = "json_utils_test",
    srcs = ["json_utils_test.cc"],
    deps = [
        ":json_utils",
        "//statechart/proto:state_machine_context_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)
