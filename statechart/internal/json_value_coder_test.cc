// Copyright 2018 The StateChart Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "statechart/internal/json_value_coder.h"

#include <limits>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "statechart/platform/types.h"
#include "statechart/platform/protobuf.h"
#include "statechart/platform/test_util.h"
#include "statechart/internal/testing/json_value_coder_test.pb.h"

namespace state_chart {
namespace {

using ::proto2::contrib::parse_proto::ParseTextOrDie;
using ::testing::ElementsAre;
using ::testing::EqualsProto;
using ::testing::SizeIs;

string ToTextFormatString(const proto2::Message& message) {
  string result;
  ::proto2::TextFormat::PrintToString(message, &result);
  return result;
}

TEST(JsonValueCoder, PODTest) {
  // For int
  {
    // Decoding
    int result = 0;
    EXPECT_TRUE(JsonValueCoder<int>::FromJsonValue(Json::Value(42), &result));
    EXPECT_EQ(42, result);

    // Encoding
    Json::Value encoded_value;
    EXPECT_TRUE(JsonValueCoder<int>::ToJsonValue(32, &encoded_value));
    EXPECT_EQ(Json::Value(32), encoded_value);
  }

  // For uint
  {
    // Decoding
    uint result = 0;
    EXPECT_TRUE(JsonValueCoder<uint>::FromJsonValue(
        Json::Value(std::numeric_limits<uint>::max()), &result));
    EXPECT_EQ(std::numeric_limits<uint>::max(), result);

    // Encoding
    Json::Value encoded_value;
    EXPECT_TRUE(JsonValueCoder<uint>::ToJsonValue(
        std::numeric_limits<uint>::max(), &encoded_value));
    EXPECT_EQ(Json::Value(std::numeric_limits<uint>::max()), encoded_value);
  }

  // For float
  {
    // Decoding
    float result = 0;
    EXPECT_TRUE(
        JsonValueCoder<float>::FromJsonValue(Json::Value(42.5), &result));
    EXPECT_EQ(42.5, result);

    // Encoding
    Json::Value encoded_value;
    EXPECT_TRUE(JsonValueCoder<float>::ToJsonValue(32.5, &encoded_value));
    EXPECT_EQ(Json::Value(32.5), encoded_value);
  }

  // For double
  {
    // Decoding
    double result = 0;
    EXPECT_TRUE(
        JsonValueCoder<double>::FromJsonValue(Json::Value(42.5), &result));
    EXPECT_EQ(42.5, result);

    // Encoding
    Json::Value encoded_value;
    EXPECT_TRUE(JsonValueCoder<double>::ToJsonValue(32.5, &encoded_value));
    EXPECT_EQ(Json::Value(32.5), encoded_value);
  }

  // For bool
  {
    // Decoding
    bool result = false;
    EXPECT_TRUE(
        JsonValueCoder<bool>::FromJsonValue(Json::Value(true), &result));
    EXPECT_EQ(true, result);

    // Encoding
    Json::Value encoded_value;
    EXPECT_TRUE(JsonValueCoder<bool>::ToJsonValue(true, &encoded_value));
    EXPECT_EQ(Json::Value(true), encoded_value);
  }
}

TEST(JsonValueCoder, ConstCharPtrPODTest) {
  // Decoding will cause a compile error.
  // const char* result;
  // JsonValueCoder<const char*>::FromJsonValue(Json::Value("hello"), &result);

  // Encoding is allowed as we can copy the 'const char*' into 'Json::Value'.
  Json::Value encoded_value;
  EXPECT_TRUE(JsonValueCoder<const char*>::ToJsonValue("abc", &encoded_value));
  EXPECT_EQ(Json::Value("abc"), encoded_value);
}

TEST(JsonValueCoder, StringTest) {
  // Decoding
  string result;
  EXPECT_TRUE(JsonValueCoder<string>::FromJsonValue(Json::Value("Hello world"),
                                                    &result));
  EXPECT_EQ("Hello world", result);

  // Encoding
  Json::Value encoded_value;
  EXPECT_TRUE(JsonValueCoder<string>::ToJsonValue("abc", &encoded_value));
  EXPECT_EQ(Json::Value("abc"), encoded_value);
}

TEST(JsonValueCoder, VectorTest) {
  // Decoding ----------------------------------------------------------------

  // Non-empty vector.
  {
    Json::Value val;
    val.append(Json::Value("one"));
    val.append(Json::Value("two"));

    std::vector<string> result;
    EXPECT_TRUE(
        JsonValueCoder<std::vector<string>>::FromJsonValue(val, &result));
    EXPECT_THAT(result, ElementsAre("one", "two"));
  }

  // Empty vector.
  {
    Json::Value val(Json::arrayValue);
    std::vector<string> result;
    EXPECT_TRUE(
        JsonValueCoder<std::vector<string>>::FromJsonValue(val, &result));
    EXPECT_THAT(result, SizeIs(0));
  }

  // Null value.
  {
    Json::Value val;
    std::vector<string> result;
    EXPECT_FALSE(
        JsonValueCoder<std::vector<string>>::FromJsonValue(val, &result));
  }

  // Encoding -----------------------------------------------------------------

  // Empty vector
  {
    Json::Value encoded;
    EXPECT_FALSE(encoded.isArray());  // Not an array value now.
    EXPECT_TRUE(
        JsonValueCoder<std::vector<string>>::ToJsonValue({}, &encoded));
    EXPECT_TRUE(encoded.isArray());  // Coerced into a array value.
    EXPECT_EQ(0, encoded.size());
  }

  // Non-empty vector
  {
    Json::Value kMissingJsonValue("missing");
    Json::Value encoded;
    EXPECT_TRUE(JsonValueCoder<std::vector<string>>::ToJsonValue({"uno", "dos"},
                                                                 &encoded));
    EXPECT_TRUE(encoded.isArray());
    EXPECT_EQ(2, encoded.size());
    EXPECT_EQ("uno",
              encoded.get(Json::ArrayIndex(0), kMissingJsonValue).asString());
    EXPECT_EQ("dos",
              encoded.get(Json::ArrayIndex(1), kMissingJsonValue).asString());
  }
}


TEST(JsonValueCoder, ProtoTest) {
  // Setting expected values.
  Json::Value json_value;
  ASSERT_TRUE(Json::Reader().parse(R"(
                                   {
                                     "aBool" : false,
                                     "aRequiredString" : "required",
                                     "testB" : {
                                       "aInt32" : 32,
                                       "aInt64" : "1099511627775",
                                       "aFloat" : 0.45,
                                       "aDouble" : 0.45,
                                       "aString" : "hello world!"
                                     }
                                   })",
                                   json_value, false));
  const TestA test_a = ParseTextOrDie<TestA>(R"(
                           a_bool: false
                           a_required_string : 'required'
                           test_b {
                             a_int32: 32
                             a_int64: 0xFFFFFFFFFF
                             a_float: 0.45
                             a_double: 0.45
                             a_string: 'hello world!'
                           })");

  // Decoding
  TestA result;
  EXPECT_TRUE(JsonValueCoder<TestA>::FromJsonValue(json_value, &result));
  EXPECT_THAT(result, EqualsProto(ToTextFormatString(test_a)));

  // Encoding
  Json::Value encoded_value;
  EXPECT_TRUE(JsonValueCoder<TestA>::ToJsonValue(test_a, &encoded_value));
  // There is no good way to compare Json::Value.
  // We can do
  //   EXPECT_EQ(json_value, encoded_value);
  // or we can use string representations like the following,
  //   EXPECT_EQ(Json::FastWriter().write(json_value),
  //             Json::FastWriter().write(encoded_value));
  // but both cannot do an approximate comparison as required for float/double.
  // Here we re-decode the encoded_value into a proto and use standard proto
  // matchers.
  TestA re_decoded_test_a;
  EXPECT_TRUE(
      JsonValueCoder<TestA>::FromJsonValue(encoded_value, &re_decoded_test_a));
  EXPECT_THAT(re_decoded_test_a, EqualsProto(ToTextFormatString(test_a)));
}

TEST(JsonValueListCoder, FromJsonTest) {
  Json::Value input1("Hello");
  Json::Value input2("World!");
  {
    std::tuple<string> result_tuple;
    EXPECT_TRUE(JsonValueListCoder::FromJson(
        std::vector<const Json::Value*>{&input1}, &result_tuple));
    EXPECT_EQ(std::make_tuple("Hello"), result_tuple);
  }
  {
    std::tuple<string, string> result_tuple;
    EXPECT_TRUE(JsonValueListCoder::FromJson(
        std::vector<const Json::Value*>{&input1, &input2}, &result_tuple));
    EXPECT_EQ(std::make_tuple("Hello", "World!"), result_tuple);
  }
  {
    Json::Value input3(41);
    std::tuple<string, int> result_tuple;
    EXPECT_TRUE(JsonValueListCoder::FromJson(
        std::vector<const Json::Value*>{&input1, &input3}, &result_tuple));
    EXPECT_EQ(std::make_tuple("Hello", 41), result_tuple);
  }
  {
    // Empty tuple from empty vector.
    std::tuple<> result_tuple;
    EXPECT_TRUE(JsonValueListCoder::FromJson(std::vector<const Json::Value*>{},
                                             &result_tuple));
    EXPECT_EQ(std::make_tuple(), result_tuple);
  }
}

}  // namespace
}  // namespace state_chart
