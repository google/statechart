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
#   Package that contains the model for a StateChart. It has classes for every
#   tag in SCXML standard.

package(default_visibility = ["//visibility:public"])

# Convenience target so people can depend on ...state_chart/internal/model
# and get all elements.
cc_library(
    name = "model",
    hdrs = ["model.h"],
    deps = [
        ":assign",
        ":data",
        ":executable_block",
        ":executable_content",
        ":for_each",
        ":if",
        ":log",
        ":model_element",
        ":raise",
        ":send",
        ":state",
        ":transition",
    ],
)

cc_library(
    name = "assign",
    srcs = ["assign.cc"],
    hdrs = ["assign.h"],
    deps = [
        ":executable_content",
        "//statechart/internal:datamodel",
        "//statechart/internal:runtime",
        "//statechart/internal:utility",
        "//statechart/platform:types",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "assign_test",
    size = "small",
    srcs = ["assign_test.cc"],
    deps = [
        ":assign",
        "//statechart/internal/testing:mock_datamodel",
        "//statechart/internal/testing:mock_runtime",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "data",
    srcs = ["data.cc"],
    hdrs = ["data.h"],
    deps = [
        ":executable_content",
        "//statechart/internal:datamodel",
        "//statechart/internal:runtime",
        "//statechart/internal:utility",
        "//statechart/platform:types",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "data_test",
    srcs = ["data_test.cc"],
    deps = [
        ":data",
        "//statechart/internal/testing:mock_datamodel",
        "//statechart/internal/testing:mock_runtime",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "executable_block",
    hdrs = ["executable_block.h"],
    deps = [
        ":executable_content",
    ],
)

cc_test(
    name = "executable_block_test",
    size = "small",
    srcs = ["executable_block_test.cc"],
    deps = [
        ":executable_block",
        "//statechart/internal/testing:mock_executable_content",
        "//statechart/internal/testing:mock_runtime",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "executable_content",
    hdrs = ["executable_content.h"],
    deps = [":model_element"],
)

cc_library(
    name = "for_each",
    srcs = ["for_each.cc"],
    hdrs = ["for_each.h"],
    deps = [
        ":executable_content",
        "//statechart/internal:datamodel",
        "//statechart/internal:runtime",
        "//statechart/internal:utility",
        "//statechart/platform:types",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "for_each_test",
    size = "small",
    srcs = ["for_each_test.cc"],
    deps = [
        ":for_each",
        "//statechart/internal/testing:mock_datamodel",
        "//statechart/internal/testing:mock_executable_content",
        "//statechart/internal/testing:mock_runtime",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "if",
    srcs = ["if.cc"],
    hdrs = ["if.h"],
    deps = [
        ":executable_content",
        "//statechart:logging",
        "//statechart/internal:datamodel",
        "//statechart/internal:runtime",
        "//statechart/internal:utility",
        "//statechart/platform:types",
        "@com_google_absl//absl/strings",
    ],
)

cc_test(
    name = "if_test",
    size = "small",
    srcs = ["if_test.cc"],
    deps = [
        ":if",
        "//statechart/internal/testing:mock_datamodel",
        "//statechart/internal/testing:mock_executable_content",
        "//statechart/internal/testing:mock_runtime",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "model_element",
    srcs = ["model_element.cc"],
    hdrs = ["model_element.h"],
)

cc_library(
    name = "log",
    srcs = ["log.cc"],
    hdrs = ["log.h"],
    deps = [
        ":executable_content",
        "//statechart/internal:datamodel",
        "//statechart/internal:runtime",
        "//statechart/internal:utility",
        "//statechart/platform:types",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "log_test",
    size = "small",
    srcs = ["log_test.cc"],
    deps = [
        ":log",
        "//statechart/internal/testing:mock_datamodel",
        "//statechart/internal/testing:mock_runtime",
        "//statechart/platform:test_util",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "raise",
    srcs = ["raise.cc"],
    hdrs = ["raise.h"],
    deps = [
        ":executable_content",
        "//statechart/internal:runtime",
        "//statechart/platform:types",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "raise_test",
    size = "small",
    srcs = ["raise_test.cc"],
    deps = [
        ":raise",
        "//statechart/internal/testing:mock_runtime",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "send",
    srcs = ["send.cc"],
    hdrs = ["send.h"],
    deps = [
        ":executable_content",
        ":str_or_expr",
        "//statechart:logging",
        "//statechart/internal:datamodel",
        "//statechart/internal:event_dispatcher",
        "//statechart/internal:runtime",
        "//statechart/internal:utility",
        "//statechart/platform:map_util",
        "//statechart/platform:types",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
    ],
)

cc_test(
    name = "send_test",
    size = "small",
    srcs = ["send_test.cc"],
    deps = [
        ":send",
        "//statechart/internal/testing:mock_datamodel",
        "//statechart/internal/testing:mock_event_dispatcher",
        "//statechart/internal/testing:mock_runtime",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "state",
    srcs = ["state.cc"],
    hdrs = ["state.h"],
    deps = [
        ":model_element",
        ":transition",
        "//statechart:logging",
    ],
)

cc_library(
    name = "str_or_expr",
    srcs = ["str_or_expr.cc"],
    hdrs = ["str_or_expr.h"],
    deps = [
        "//statechart/internal:datamodel",
        "//statechart/platform:types",
    ],
)

cc_test(
    name = "str_or_expr_test",
    size = "small",
    srcs = ["str_or_expr_test.cc"],
    deps = [
        ":str_or_expr",
        "//statechart/internal/testing:mock_datamodel",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "transition",
    srcs = ["transition.cc"],
    hdrs = ["transition.h"],
    deps = [
        ":model_element",
        ":state_hdr",
        "//statechart/internal:datamodel",
        "//statechart/internal:runtime",
        "//statechart/internal:utility",
        "//statechart/platform:types",
        "@com_google_absl//absl/strings",
    ],
)

# This header-only library was inserted because the target
#   //statechart/internal/model:transition, which includes src/internal/model/state.h
# cannot depend on the library //statechart/internal/model:state.
cc_library(
    name = "state_hdr",
    hdrs = ["state.h"],
    visibility = ["//visibility:private"],
    deps = [
        ":model_element",
        "//statechart/platform:types",
    ],
)

cc_test(
    name = "transition_test",
    size = "small",
    srcs = ["transition_test.cc"],
    deps = [
        ":transition",
        "//statechart/internal/testing:mock_runtime",
        "@com_google_googletest//:gtest_main",
    ],
)
