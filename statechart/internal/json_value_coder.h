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

// Coding and Decoding routines from Json::Value to any data type.
// Not all data types conversions are implemented yet.

#ifndef STATE_CHART_INTERNAL_JSON_VALUE_CODER_H_
#define STATE_CHART_INTERNAL_JSON_VALUE_CODER_H_

#include <sys/types.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <glog/logging.h>

#include "statechart/platform/types.h"
#include "absl/strings/str_cat.h"
#include "include/json/json.h"
#include "statechart/platform/logging.h"
#include "statechart/platform/map_util.h"
#include "statechart/platform/protobuf.h"

namespace state_chart {

// Every JsonValueCoder implementation should have the following methods.
//   static void Encode(const Type& value, Json::Value* result);
//   static bool Decode(const Json::Value& value, Type* result);
template <typename Type>
class JsonValueCoderCustom;

// JsonValueCoderPOD has implementations for int, uint, int64, uint64.
// float, double, bool.
// Template specializations are provided for all above-mentioned POD decoders.
template <typename Type>
class JsonValueCoderPOD;

template <typename Type>
class JsonValueCoder {
 public:
  // Convert a value of type 'Type' to a Json::Value in result
  static bool ToJsonValue(const Type& value, Json::Value* result) {
    return ImplType::Encode(value, result);
  }

  static bool FromJsonValue(const Json::Value& value, Type* result) {
    return ImplType::Decode(value, result);
  }

 private:
  // Inner class for de/encoding protobuf values.
  class JsonValueCoderProtocolMessage {
   public:
    static bool Encode(const Type& value, Json::Value* result);
    static bool Decode(const Json::Value& value, Type* result);

   private:
    static gtl::LazyStaticPtr<proto2::util::JsonFormat, int64> json_format_ptr_;
  };

  // Coding is possible for ProtocolMessages, proto2::Messages,
  // PODs and others that where implemented (such as vectors of PODs).
  // Thus determine how the coding is to take place:
  // If Type is derived from ProtocolMessage or proto2::Message,
  // use JsonValueCoderProtocolMessage.
  // If Type is a POD, use JsonValueCoderPOD.
  // Otherwise, use JsonValueCoderCustom (for which the coding hopefully has
  // been implemented).
  typedef typename std::conditional<
      std::is_convertible<Type*, proto2::MessageLite*>::value,
      JsonValueCoderProtocolMessage,
      typename std::conditional<
          std::is_pod<Type>::value, JsonValueCoderPOD<Type>,
          JsonValueCoderCustom<Type>>::type>::type ImplType;
};

template <typename Type>
gtl::LazyStaticPtr<proto2::util::JsonFormat, int64>
    JsonValueCoder<Type>::JsonValueCoderProtocolMessage::json_format_ptr_ = {
        proto2::util::JsonFormat::ADD_WHITESPACE |
        proto2::util::JsonFormat::QUOTE_LARGE_INTS |
        proto2::util::JsonFormat::USE_JSON_OPT_PARAMETERS};

template <typename Type>
bool JsonValueCoder<Type>::JsonValueCoderProtocolMessage::Encode(
    const Type& value, Json::Value* result) {
  string json_string;
  json_format_ptr_->PrintToString(value, &json_string);
  Json::Reader reader;
  const bool success =
      reader.parse(json_string, *result, false /* collectComments */);
  LOG_IF(ERROR, !success) << "Failed in reading as Json::Value. Error: "
                          << reader.getFormattedErrorMessages()
                          << "\nValue : " << json_string;
  return success && result->isObject();
}

template <typename Type>
bool JsonValueCoder<Type>::JsonValueCoderProtocolMessage::Decode(
    const Json::Value& value, Type* result) {
  if (!value.isObject()) {
    LOG(INFO) << "Trying to convert a non-object Json::value to proto. value:"
              << Json::StyledWriter().write(value);
    return false;
  }
  string json_string = Json::FastWriter().write(value);
  return json_format_ptr_->ParseFromString(json_string, result);
}

// A convenience macro for defining template specializations for
// JsonValueCoderPOD class.
#define DEFINE_JSON_VALUE_CODER_POD(Type, JsonFunctionNameSuffix) \
  template <>                                                     \
  class JsonValueCoderPOD<Type> {                                 \
   public:                                                        \
    static bool Encode(const Type& value, Json::Value* result) {  \
      *result = Json::Value(value);                               \
      return true;                                                \
    }                                                             \
                                                                  \
    static bool Decode(const Json::Value& value, Type* result) {  \
      const bool success = value.is##JsonFunctionNameSuffix();    \
      if (success) {                                              \
        *result = value.as##JsonFunctionNameSuffix();             \
      }                                                           \
      return success;                                             \
    }                                                             \
  };

DEFINE_JSON_VALUE_CODER_POD(int, Int)
DEFINE_JSON_VALUE_CODER_POD(uint, UInt)
DEFINE_JSON_VALUE_CODER_POD(int64, Int64)
DEFINE_JSON_VALUE_CODER_POD(float, Double)
DEFINE_JSON_VALUE_CODER_POD(double, Double)
DEFINE_JSON_VALUE_CODER_POD(bool, Bool)

#undef DECLARE_JSON_VALUE_CODER_POD

// Encoding 'const char*' is supported. Compilation will fail when decoding
// 'const char*'. Decoding is not supported as the Json::Value may be a
// temporary value (e.g. an r-value).
template <>
class JsonValueCoderPOD<const char*> {
 public:
  static bool Encode(const char* value, Json::Value* result) {
    *result = Json::Value(value);
    return true;
  }
};

// For string
template <>
class JsonValueCoderCustom<string> {
 public:
  static bool Encode(const string& value, Json::Value* result) {
    *result = Json::Value(value);
    return true;
  }

  static bool Decode(const Json::Value& value, string* result) {
    const bool success = value.isString();
    if (success) {
      result->assign(value.asString());
    }
    return success;
  }
};

// For Json::Value (identity)
template <>
class JsonValueCoderCustom<Json::Value> {
 public:
  static bool Encode(const Json::Value& value, Json::Value* result) {
    *result = value;
    return true;
  }

  static bool Decode(const Json::Value& value, Json::Value* result) {
    *result = value;
    return true;
  }
};

// For vector<Type>
template <typename T, typename Allocator>
class JsonValueCoderCustom<std::vector<T, Allocator>> {
 public:
  static bool Encode(const std::vector<T, Allocator>& value,
                     Json::Value* result) {
    *result = Json::Value(Json::arrayValue);  // Initialize as an array.
    for (const auto& i : value) {
      Json::Value v;
      if (!JsonValueCoder<T>::ToJsonValue(i, &v)) {
        return false;
      }
      result->append(v);
    }
    return true;
  }

  static bool Decode(const Json::Value& value,
                     std::vector<T, Allocator>* result) {
    if (!value.isArray()) {
      return false;
    }
    for (Json::ArrayIndex i = 0; i < value.size(); ++i) {
      T new_val;
      if (!JsonValueCoder<T>::FromJsonValue(value.get(i, Json::Value()),
                                            &new_val)) {
        return false;
      }
      result->push_back(new_val);
    }
    return true;
  }
};

// TODO(srgandhe): See if we can use util/tuple/generate.h
class JsonValueListCoder {
 public:
  template <typename First>
  static bool FromJson(const std::vector<const Json::Value*>& values,
                       std::tuple<First>* tuple) {
    if (values.size() != 1) {
      LOG(INFO) << "mismatch in the argument size";
      return false;
    }
    static_assert(!std::is_const<First>::value,
                  "Cannot decode a Json::Value to a const.");
    static_assert(!std::is_reference<First>::value,
                  "Cannot decode a Json::Value to a reference.");
    static_assert(!std::is_pointer<First>::value,
                  "Cannot decode a Json::Value to a pointer.");
    First first;
    if (!JsonValueCoder<First>::FromJsonValue(*values.at(0), &first)) {
      return false;
    }
    *tuple = std::make_tuple(first);
    return true;
  }

  template <typename First, typename Second, typename... Rest>
  static bool FromJson(const std::vector<const Json::Value*>& values,
                       std::tuple<First, Second, Rest...>* tuple) {
    if (values.size() != 2 + sizeof...(Rest)) {
      LOG(INFO) << "mismatch in the argument size";
      return false;
    }
    std::vector<const Json::Value*> first_value{values.at(0)};
    std::vector<const Json::Value*> rest_values(values.begin() + 1,
                                                values.end());
    std::tuple<First> first_tuple;
    std::tuple<Second, Rest...> rest_tuple;
    if (!FromJson(first_value, &first_tuple) ||
        !FromJson(rest_values, &rest_tuple)) {
      return false;
    }
    *tuple = std::tuple_cat(first_tuple, rest_tuple);
    return true;
  }

  static bool FromJson(const std::vector<const Json::Value*>& values,
                       std::tuple<>* tuple) {
    if (values.size() != 0) {
      LOG(INFO) << "mismatch in the argument size";
      return false;
    }
    *tuple = std::make_tuple();
    return true;
  }
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_JSON_VALUE_CODER_H_
