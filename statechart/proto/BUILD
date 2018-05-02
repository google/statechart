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

proto_library(
    name = "state_chart_proto",
    srcs = ["state_chart.proto"],
)

proto_library(
    name = "state_machine_context_proto",
    srcs = ["state_machine_context.proto"],
)

cc_proto_library(
    name = "state_chart_cc_proto",
    deps = [":state_chart_proto"],
)

cc_proto_library(
    name = "state_machine_context_cc_proto",
    deps = [":state_machine_context_proto"],
)
