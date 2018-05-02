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

# A list of BUILD targets that are specific to platform being used.

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "types",
    hdrs = ["types.h"],
)

cc_library(
    name = "logging",
    hdrs = ["logging.h"],
)

cc_library(
    name = "map_util",
    hdrs = ["map_util.h"],
    deps = [
        "@com_google_absl//absl/synchronization",
        "@com_google_glog//:glog",
    ],
)

cc_library(
    name = "str_util",
    srcs = ["str_util.cc"],
    hdrs = ["str_util.h"],
    deps = [
        ":types",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "protobuf",
    hdrs = ["protobuf.h"],
    deps = [
        ":types",
        "@com_google_glog//:glog",
        "@com_google_protobuf//:protobuf",
    ],
)

cc_library(
    name = "test_util",
    testonly = 1,
    hdrs = ["test_util.h"],
    deps = [
        ":protobuf",
        "@com_google_glog//:glog",
        "@com_google_googletest//:gtest",
    ],
)
