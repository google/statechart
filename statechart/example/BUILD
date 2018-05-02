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

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "microwave_example_main",
    srcs = ["microwave_example_main.cc"],
    deps = [
        ":microwave_cc_proto",
        "//statechart:state_machine",
        "//statechart:state_machine_factory",
        "//statechart/internal:function_dispatcher_impl",
        "//statechart/platform:protobuf",
        "//statechart/proto:state_chart_cc_proto",
        "//statechart/proto:state_machine_context_cc_proto",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_absl//absl/memory",
        "@com_google_absl//absl/strings",
        "@com_google_glog//:glog",
    ],
)

proto_library(
    name = "microwave_proto",
    srcs = ["microwave.proto"],
)

cc_proto_library(
    name = "microwave_cc_proto",
    deps = [":microwave_proto"],
)
