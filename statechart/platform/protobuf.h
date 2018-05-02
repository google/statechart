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

#ifndef STATECHART_PLATFORM_PROTOBUF_H_
#define STATECHART_PLATFORM_PROTOBUF_H_

#include "google/protobuf/arena.h"
#include "google/protobuf/compiler/importer.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/zero_copy_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/map.h"
#include "google/protobuf/repeated_field.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/json_util.h"
#include "google/protobuf/util/type_resolver_util.h"
#include "google/protobuf/util/message_differencer.h"
#include "google/protobuf/stubs/status.h"

#include "statechart/platform/types.h"

#include <glog/logging.h>

using ::google::int64;

namespace proto2 {
using ::google::protobuf::RepeatedPtrField;
using ::google::protobuf::RepeatedField;
using ::google::protobuf::TextFormat;
using ::google::protobuf::Message;
using ::google::protobuf::MessageLite;
using ::google::protobuf::Descriptor;
using ::google::protobuf::DescriptorPool;
using ::google::protobuf::FieldDescriptor;

namespace util {

using ::google::protobuf::util::MessageDifferencer;
using ::google::protobuf::util::JsonStringToMessage;
using ::google::protobuf::util::MessageToJsonString;

class JsonFormat {
 public:

  enum Flags {
    ADD_WHITESPACE = 1 << 0,
    SYMBOLIC_ENUMS = 1 << 3,
    QUOTE_LARGE_INTS = 1 << 7,
    USE_JSON_OPT_PARAMETERS = 1 << 11,
  };

  explicit JsonFormat(int64 flags) {};
  virtual ~JsonFormat() = default;

  bool ParseFromString(const string& input, Message* output) const {
    google::protobuf::util::Status status = JsonStringToMessage(input, output);
    CHECK(status.ok()) << status.ToString();
    return status.ok();
  }

  bool PrintToString(const Message& message, string* output) const {
    google::protobuf::util::Status status = MessageToJsonString(message, output);
    CHECK(status.ok()) << status.ToString();
    return status.ok();
  }
};

}  // namespace util

namespace contrib {
namespace parse_proto {

template <typename T>
T ParseTextOrDie(const string& input) {
  T result;
  CHECK(proto2::TextFormat::ParseFromString(input, &result));
  return result;
}

}  // namespace parse_proto
}  // namespace contrib


}  // namespace proto2

#endif  // STATECHART_PLATFORM_PROTOBUF_H_
