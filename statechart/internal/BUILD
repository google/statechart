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
#   Internal libraries that run StateChart specification.

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "datamodel",
    srcs = ["datamodel.cc"],
    hdrs = ["datamodel.h"],
    deps = [
        "//statechart/platform:types",
    ],
)

cc_library(
    name = "executor",
    srcs = ["executor.cc"],
    hdrs = ["executor.h"],
    deps = [
        ":datamodel",
        ":event_dispatcher",
        ":model",
        ":runtime",
        ":utility",
        "//statechart:logging",
        "//statechart/internal/model",
        "//statechart/platform:types",
        "//statechart/proto:state_chart_cc_proto",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "executor_test",
    size = "small",
    srcs = ["executor_test.cc"],
    deps = [
        ":executor",
        "//statechart/internal/testing:delegating_mock_executor",
        "//statechart/internal/testing:mock_datamodel",
        "//statechart/internal/testing:mock_event_dispatcher",
        "//statechart/internal/testing:mock_executable_content",
        "//statechart/internal/testing:mock_model",
        "//statechart/internal/testing:mock_runtime",
        "//statechart/internal/testing:mock_state",
        "//statechart/internal/testing:mock_transition",
        "//statechart/platform:types",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "event_dispatcher",
    srcs = ["event_dispatcher.cc"],
    hdrs = ["event_dispatcher.h"],
    deps = [
        ":runtime",
        "//statechart:state_machine_listener",
        "//statechart/platform:types",
    ],
)

cc_test(
    name = "event_dispatcher_test",
    size = "small",
    srcs = ["event_dispatcher_test.cc"],
    deps = [
        ":event_dispatcher",
        "//statechart:state_machine_listener",
        "//statechart/internal/testing:mock_runtime",
        "//statechart/internal/testing:mock_state",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "function_dispatcher",
    hdrs = ["function_dispatcher.h"],
    visibility = ["//visibility:public"],
    deps = [
        "//statechart/platform:types",
    ],
)

cc_library(
    name = "function_dispatcher_builtin",
    srcs = ["function_dispatcher_builtin.cc"],
    hdrs = ["function_dispatcher_builtin.h"],
    deps = [
        "//statechart/platform:types",
        "@jsoncpp_git//:jsoncpp",
    ],
)

cc_test(
    name = "function_dispatcher_builtin_test",
    srcs = ["function_dispatcher_builtin_test.cc"],
    deps = [
        ":function_dispatcher_builtin",
        "//statechart/platform:types",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
        "@jsoncpp_git//:jsoncpp",
    ],
)

cc_library(
    name = "function_dispatcher_impl",
    srcs = ["function_dispatcher_impl.cc"],
    hdrs = ["function_dispatcher_impl.h"],
    deps = [
        ":function_dispatcher",
        ":function_dispatcher_builtin",
        ":json_value_coder",
        ":utility",
        "//statechart/platform:map_util",
        # "//statechart/platform:tuple_util",
        "//statechart/platform:types",
        "@com_google_absl//absl/utility",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@jsoncpp_git//:jsoncpp",
    ],
)

cc_test(
    name = "function_dispatcher_impl_test",
    srcs = ["function_dispatcher_impl_test.cc"],
    deps = [
        ":function_dispatcher_impl",
        "//statechart/platform:types",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
        "@jsoncpp_git//:jsoncpp",
    ],
)

cc_library(
    name = "json_value_coder",
    hdrs = ["json_value_coder.h"],
    deps = [
        "//statechart/platform:logging",
        "//statechart/platform:map_util",
        "//statechart/platform:protobuf",
        "//statechart/platform:types",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
        "@jsoncpp_git//:jsoncpp",
    ],
)

cc_test(
    name = "json_value_coder_test",
    size = "small",
    srcs = ["json_value_coder_test.cc"],
    deps = [
        ":json_value_coder",
        "//statechart/internal/testing:json_value_coder_test_cc_proto",
        "//statechart/platform:protobuf",
        "//statechart/platform:test_util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "light_weight_datamodel",
    srcs = ["light_weight_datamodel.cc"],
    hdrs = ["light_weight_datamodel.h"],
    deps = [
        ":datamodel",
        ":function_dispatcher",
        ":runtime",
        ":utility",
        "//statechart:logging",
        "//statechart/platform:map_util",
        "//statechart/platform:str_util",
        "//statechart/platform:types",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@jsoncpp_git//:jsoncpp",
    ],
)

cc_test(
    name = "light_weight_datamodel_test",
    size = "small",
    srcs = ["light_weight_datamodel_test.cc"],
    deps = [
        ":light_weight_datamodel",
        "//statechart/internal/testing:mock_function_dispatcher",
        "//statechart/internal/testing:mock_runtime",
        "//statechart/platform:types",
        "@com_google_absl//absl/strings",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "model_builder",
    srcs = ["model_builder.cc"],
    hdrs = ["model_builder.h"],
    deps = [
        ":model_impl",
        "//statechart:logging",
        "//statechart/internal/model",
        "//statechart/platform:map_util",
        "//statechart/platform:protobuf",
        "//statechart/proto:state_chart_cc_proto",
    ],
)

cc_test(
    name = "model_builder_test",
    size = "small",
    srcs = ["model_builder_test.cc"],
    deps = [
        ":model",
        ":model_builder",
        "//statechart/internal/model",
        "//statechart/internal/testing:mock_executable_content",
        "//statechart/internal/testing:mock_runtime",
        "//statechart/internal/testing:mock_state",
        "//statechart/internal/testing:mock_transition",
        "//statechart/internal/testing:state_chart_builder",
        "//statechart/platform:map_util",
        "//statechart/platform:protobuf",
        "//statechart/platform:test_util",
        "//statechart/proto:state_chart_cc_proto",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "model_impl",
    srcs = ["model_impl.cc"],
    hdrs = ["model_impl.h"],
    deps = [
        ":model",
        ":runtime",
        "//statechart:logging",
        "//statechart/internal/model",
        "//statechart/platform:logging",
        "//statechart/platform:map_util",
        "//statechart/platform:protobuf",
        "//statechart/platform:types",
        "//statechart/proto:state_chart_cc_proto",
        "//statechart/proto:state_machine_context_cc_proto",
        "@com_google_absl//absl/algorithm:container",
    ],
)

cc_test(
    name = "model_impl_test",
    size = "small",
    srcs = ["model_impl_test.cc"],
    deps = [
        ":model_impl",
        "//statechart/internal/model",
        "//statechart/internal/testing:mock_runtime",
        "//statechart/internal/testing:mock_state",
        "//statechart/internal/testing:mock_transition",
        "//statechart/platform:test_util",
        "//statechart/platform:types",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "model",
    srcs = ["model.cc"],
    hdrs = ["model.h"],
    deps = [
        "//statechart/platform:protobuf",
        "//statechart/platform:types",
        "//statechart/proto:state_chart_cc_proto",
        "//statechart/proto:state_machine_context_cc_proto",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "model_test",
    size = "small",
    srcs = ["model_test.cc"],
    deps = [
        ":model",
        "//statechart/platform:types",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "runtime_impl",
    srcs = ["runtime_impl.cc"],
    hdrs = ["runtime_impl.h"],
    deps = [
        ":datamodel",
        ":event_dispatcher",
        ":runtime",
        "//statechart:logging",
        "//statechart/internal/model",
        "//statechart/platform:map_util",
        "//statechart/platform:protobuf",
        "//statechart/platform:types",
        "//statechart/proto:state_machine_context_cc_proto",
        "@com_google_absl//absl/algorithm:container",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "runtime_impl_test",
    size = "small",
    srcs = ["runtime_impl_test.cc"],
    deps = [
        ":runtime_impl",
        "//statechart/internal/testing:mock_datamodel",
        "//statechart/internal/testing:mock_state",
        "//statechart/platform:test_util",
        "@com_google_absl//absl/memory",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "runtime",
    srcs = ["runtime.cc"],
    hdrs = ["runtime.h"],
    deps = [
        ":utility",
        "//statechart/platform:types",
        "//statechart/proto:state_machine_context_cc_proto",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "state_machine_impl",
    srcs = ["state_machine_impl.cc"],
    hdrs = ["state_machine_impl.h"],
    deps = [
        ":event_dispatcher",
        ":executor",
        ":light_weight_datamodel",
        ":model",
        ":runtime",
        "//statechart:state_machine",
        "//statechart/platform:types",
        "@com_google_absl//absl/memory",
    ],
)

cc_library(
    name = "state_machine_logger",
    srcs = ["state_machine_logger.cc"],
    hdrs = ["state_machine_logger.h"],
    deps = [
        ":datamodel",
        ":runtime",
        "//statechart:state_machine_listener",
        "//statechart/internal/model",
        "//statechart/platform:types",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "utility",
    srcs = ["utility.cc"],
    hdrs = ["utility.h"],
    deps = [
        "//statechart/platform:str_util",
        "//statechart/platform:types",
        "@com_google_absl//absl/strings",
        "@com_googlesource_code_re2//:re2",
    ],
)

cc_test(
    name = "utility_test",
    size = "small",
    srcs = ["utility_test.cc"],
    deps = [
        ":utility",
        "//statechart/platform:types",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest_main",
    ],
)
