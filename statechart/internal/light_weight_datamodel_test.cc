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

#include "statechart/internal/light_weight_datamodel.h"

#include <list>
#include <memory>
#include <utility>
#include <vector>

#include <glog/logging.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "absl/memory/memory.h"
#include "absl/strings/numbers.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "statechart/internal/testing/mock_function_dispatcher.h"
#include "statechart/internal/testing/mock_runtime.h"

using ::absl::StrCat;
using ::absl::WrapUnique;
using testing::_;
using testing::DoAll;
using testing::ElementsAreArray;
using testing::Eq;
using testing::NiceMock;
using testing::Return;
using testing::SetArgPointee;

namespace state_chart {
namespace {

// Test that the tokenizer can tokenize the expressions we are supporting.
TEST(InternalTokenizeExpression, TokenizeExpression) {
  std::pair<string, std::vector<string>> cases[] = {
      // Values that are numbers.
      {"1", {"1"}},
      {"1.2", {"1.2"}},
      {" 1.2 ", {"1.2"}},
      // String values.
      {"\"ABC\"", {"\"ABC\""}},
      {"\"A B C \" ", {"\"A B C \""}},
      {"\"A \\\"B C\" ", {"\"A \\\"B C\""}},
      {"\"A 'B' C\" ", {"\"A 'B' C\""}},
      // Single quoted string values.
      {"'ABC'", {"'ABC'"}},
      {"'A B C ' ", {"'A B C '"}},
      {"'A \'B C'", {"'A \'B C'"}},
      {"'A \"B\" C'", {"'A \"B\" C'"}},
      // Locations.
      {"_foo.bar", {"_foo.bar"}},
      {" foo.bar ", {"foo.bar"}},
      {"foo ", {"foo"}},
      // Unary or Binary operations.
      {"\"A B\" + \" C D\"", {"\"A B\"", "+", "\" C D\""}},
      {"\"A\" + foo.bar", {"\"A\"", "+", "foo.bar"}},
      {"foo.bar / 0.34", {"foo.bar", "/", "0.34"}},
      {"foo.bar != bar.foo", {"foo.bar", "!=", "bar.foo"}},
      {"- 1", {"-", "1"}},
      // Mixed string quotes expressions.
      {"\"A B\" + 'C D'", {"\"A B\"", "+", "'C D'"}},
      // Function call.
      {" Math.Random() ", {"Math.Random", "(", ")"}},
      {"binary_op( a +b, c -d  )",
       {"binary_op", "(", "a", "+", "b", ",", "c", "-", "d", ")"}},
      // Array addressing.
      {"foo.bar[5]", {"foo.bar", "[", "5", "]"}},
      {"foo.bar[i+1]", {"foo.bar", "[", "i", "+", "1", "]"}},
      // Dictionary addressing.
      {"foo[\"bar\"]", {"foo", "[", "\"bar\"", "]"}},
      {"foo[\"bar\" + \"goo\"]", {"foo", "[", "\"bar\"", "+", "\"goo\"", "]"}},
      {"foo[bar]", {"foo", "[", "bar", "]"}},
      // Nested object addressing.
      {"foo[1][2]", {"foo", "[", "1", "]", "[", "2", "]"}},
      {"foo[\"bar\"][\"goo\"]",
       {"foo", "[", "\"bar\"", "]", "[", "\"goo\"", "]"}},
      {"foo[1][\"bar\"]", {"foo", "[", "1", "]", "[", "\"bar\"", "]"}},
      {"foo[1].bar[2]", {"foo", "[", "1", "]", ".bar", "[", "2", "]"}},
      {"foo[\"goo\"].bar[2]",
       {"foo", "[", "\"goo\"", "]", ".bar", "[", "2", "]"}},
      // Complex expressions.
      {"1 *2   +  3", {"1", "*", "2", "+", "3"}},
      {"A < B && ( C >= D || C <= E )",
       {"A", "<", "B", "&&", "(", "C", ">=", "D", "||", "C", "<=", "E", ")"}},
      {" - ( foo.bar - bar.foo )", {"-", "(", "foo.bar", "-", "bar.foo", ")"}},
      {"binary_op( \"str\" + Math.random(), Math.random() * 2 )",
       {"binary_op", "(", "\"str\"", "+", "Math.random", "(", ")", ",",
        "Math.random", "(", ")", "*", "2", ")"}},
      {"foo.op1(foo.op2(a - b()) + goo_op(foo.op4(bar)))",
       {"foo.op1", "(", "foo.op2", "(", "a", "-", "b", "(", ")", ")", "+",
        "goo_op", "(", "foo.op4", "(", "bar", ")", ")", ")"}},
  };

  for (const auto& test_case : cases) {
    std::list<string> tokens;
    internal::TokenizeExpression(test_case.first, &tokens);
    EXPECT_THAT(tokens, ElementsAreArray(test_case.second)) << test_case.first;
  }
}

class LightWeightDatamodelTest : public testing::Test {
 public:
  LightWeightDatamodelTest()
      : dispatcher_(new NiceMock<MockFunctionDispatcher>()),
        datamodel_(LightWeightDatamodel::Create(dispatcher_.get())) {}

  // Declare a variable, if successful, assign 'expr' to 'location'.
  bool DeclareAndAssign(const string& location, const string& expr) {
    if (!datamodel_->Declare(location)) {
      return false;
    }
    return datamodel_->AssignExpression(location, expr);
  }

 protected:
  std::unique_ptr<MockFunctionDispatcher> dispatcher_;
  std::unique_ptr<LightWeightDatamodel> datamodel_;
  MockRuntime runtime_;
};

// Test that primitive value expressions are returned the same way they are
// expressed.
TEST_F(LightWeightDatamodelTest, EvaluatePrimitiveValues) {
  string result;

  for (const auto& value :
       {"1", "2", R"("some string")", R"({"key":"value"})"}) {
    EXPECT_TRUE(datamodel_->EvaluateExpression(value, &result));
    EXPECT_EQ(value, result);
  }
}

// Test that variables that are assigned with non-object values exists and can
// be evaluated to obtain their value.
TEST_F(LightWeightDatamodelTest, AssignPrimitiveValues) {
  string result;

  // Test assign.
  EXPECT_TRUE(DeclareAndAssign("foo", "1"));
  EXPECT_TRUE(datamodel_->IsDefined("foo"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("foo", &result));
  EXPECT_EQ("1", result);

  // Test that assigning additional variables do not invalidate other variables.
  EXPECT_TRUE(DeclareAndAssign("bar", "2"));
  EXPECT_TRUE(datamodel_->IsDefined("bar"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("bar", &result));
  EXPECT_EQ("2", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("foo", &result));
  EXPECT_EQ("1", result);

  // Test that assigning to existing locations do not invalidate other
  // variables.
  EXPECT_TRUE(datamodel_->AssignExpression("foo", "3"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("foo", &result));
  EXPECT_EQ("3", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("bar", &result));
  EXPECT_EQ("2", result);
}

// Test that JSON objects can be used in assignment.
TEST_F(LightWeightDatamodelTest, AssignJSON) {
  string result;

  // Test that the locations are assigned correctly.
  // Note that bar's value is a string.
  const char kJSON1[] = R"({"bar":"1","foo":2})";
  EXPECT_TRUE(DeclareAndAssign("object", kJSON1));
  EXPECT_TRUE(datamodel_->IsDefined("object"));
  EXPECT_TRUE(datamodel_->IsDefined("object.bar"));
  EXPECT_TRUE(datamodel_->IsDefined("object.foo"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("object.bar", &result));
  EXPECT_EQ("\"1\"", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("object.foo", &result));
  EXPECT_EQ("2", result);

  // Test retrieving whole object.
  EXPECT_TRUE(datamodel_->EvaluateExpression("object", &result));
  EXPECT_EQ(kJSON1, result);

  // Test updating object member.
  EXPECT_TRUE(datamodel_->AssignExpression("object.foo", "1"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("object.foo", &result));
  EXPECT_EQ("1", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("object.bar", &result));
  EXPECT_EQ("\"1\"", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("object", &result));
  EXPECT_EQ(R"({"bar":"1","foo":1})", result);

  // Test updating entire object.
  const char kJSON2[] = R"({"bar":8,"foo":9})";
  EXPECT_TRUE(datamodel_->AssignExpression("object", kJSON2));
  EXPECT_TRUE(datamodel_->EvaluateExpression("object.foo", &result));
  EXPECT_EQ("9", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("object.bar", &result));
  EXPECT_EQ("8", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("object", &result));
  EXPECT_EQ(kJSON2, result);

  // Test assigning different object.
  EXPECT_TRUE(DeclareAndAssign("other", kJSON1));
  EXPECT_TRUE(datamodel_->EvaluateExpression("other", &result));
  EXPECT_EQ(kJSON1, result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("object", &result));
  EXPECT_EQ(kJSON2, result);

  // Test nested objects.
  EXPECT_TRUE(datamodel_->AssignExpression("object.nested", kJSON2));
  EXPECT_TRUE(datamodel_->EvaluateExpression("object.nested", &result));
  EXPECT_EQ(kJSON2, result);
  // Update a nested object field.
  EXPECT_TRUE(datamodel_->AssignExpression("object.nested.foo", "100"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("object.nested.foo", &result));
  EXPECT_EQ("100", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("object", &result));
  EXPECT_EQ(R"({"bar":8,"foo":9,"nested":{"bar":8,"foo":100}})", result);
  // Check that other object is not affected.
  EXPECT_TRUE(datamodel_->EvaluateExpression("other", &result));
  EXPECT_EQ(kJSON1, result);

  // Test updating entire object with different fields.
  const char kJSON3[] = R"({"google":"A","plus":"B"})";
  EXPECT_TRUE(datamodel_->AssignExpression("object", kJSON3));
  EXPECT_FALSE(datamodel_->EvaluateExpression("object.foo", &result));
  EXPECT_FALSE(datamodel_->EvaluateExpression("object.bar", &result));
  EXPECT_TRUE(datamodel_->EvaluateExpression("object", &result));
  EXPECT_EQ(kJSON3, result);

  // Test assigning new field to an object.
  EXPECT_TRUE(DeclareAndAssign("event", "{}"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("event", &result));
  EXPECT_EQ("{}", result);
  EXPECT_TRUE(datamodel_->AssignExpression("event.id", R"("foobar")"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("event", &result));
  EXPECT_EQ(R"({"id":"foobar"})", result);

  // Assign a JSON object with multiple newlines.
  EXPECT_TRUE(DeclareAndAssign("newlines1", "\n{\n\n}\n"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("newlines1", &result));
  EXPECT_EQ("{}", result);
  EXPECT_TRUE(DeclareAndAssign("newlines2", "\n{\n\"cow\"\n:\n2\n}\n"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("newlines2", &result));
  EXPECT_EQ("{\"cow\":2}", result);

  // Assign a JSON with complex string field name.
  const char kComplexFieldNameJSON[] =
      R"({"[complex_name]":{"bar":2},"foo":1})";
  EXPECT_TRUE(DeclareAndAssign("complex_name", kComplexFieldNameJSON));
  EXPECT_TRUE(datamodel_->EvaluateExpression("complex_name", &result));
  EXPECT_EQ(kComplexFieldNameJSON, result);

  // Assign a JSON with an array.
  const char kArrayFieldJSON[] = R"({"foo":1,"some_array":[1,2]})";
  EXPECT_TRUE(DeclareAndAssign("array_json", kArrayFieldJSON));
  EXPECT_TRUE(datamodel_->EvaluateExpression("array_json", &result));
  EXPECT_EQ(kArrayFieldJSON, result);
}

// Test the assignment of a complex JSON object with multiple nested objects.
TEST_F(LightWeightDatamodelTest, AssignJSONWithMultipleChildObjects) {
  string result;

  const string kJSON = R"({"music_fact_input":{"count":0},)"
                       R"("music_input":{"artist_mid":"/m/09889g"},)"
                       R"("music_news_input":{"count":0}})";

  // Create object with separate assignments.
  EXPECT_TRUE(
      DeclareAndAssign("_response_generator.music_fact_input.count", "0"));
  EXPECT_TRUE(DeclareAndAssign("_response_generator.music_input.artist_mid",
                               "\"/m/09889g\""));
  EXPECT_TRUE(
      DeclareAndAssign("_response_generator.music_news_input.count", "0"));
  LOG(INFO) << "Datamodel: " << datamodel_->DebugString();
  EXPECT_TRUE(datamodel_->EvaluateExpression("_response_generator", &result));
  EXPECT_EQ(kJSON, result);

  // Create object at once.
  EXPECT_TRUE(datamodel_->AssignExpression("_response_generator", kJSON));
  LOG(INFO) << "Datamodel: " << datamodel_->DebugString();
  EXPECT_TRUE(datamodel_->EvaluateExpression("_response_generator", &result));
  EXPECT_EQ(kJSON, result);
}

// Test the assignment of a complex JSON object with deeply nested object with
// arrays (that are currently unsupported).
TEST_F(LightWeightDatamodelTest, AssignJSONDeeplyNestedObjects) {
  // Deeper nested object.
  const string kJSON = R"({
  "type": 1,
  "source": {
    "role": 1
  },
  "utterance": {
    "text": "tell me about artist madonna",
    "aqua": {
      "interpretation": {
        "event_name": "U.tell_me_about_artist_x",
        "music": {
          "artist_entity": {
            "freebase_mid": "/m/01vs_v8",
            "confidence_score": 0.79515373706817627,
            "annotated_span": "madonna",
            "collection_membership": [ {
              "collection_id": "/collection/musical_artists",
              "collection_score": 0.4708310067653656
            }, {
              "collection_id": "/collection/composers",
              "collection_score": 0.3811739981174469
            }, {
              "collection_id": "/collection/lyricists",
              "collection_score": 0.23878300189971924
            } ],
            "interpretation_number": 0,
            "entity_number": 0,
            "annotated_relationship": [ ],
            "[nlp_semantic_parsing.qref_annotation_eval_data]": {
              "start_token": 4,
              "num_tokens": 1
            }
          }
        }
      }
    }
  }
})";

  EXPECT_TRUE(DeclareAndAssign("object", kJSON));
  LOG(INFO) << "Datamodel: " << datamodel_->DebugString();

  const std::pair<string, string> key_values[] = {
      {"object.type", "1"},
      {"object.source", R"({"role":1})"},
      {"object.utterance.text", "\"tell me about artist madonna\""},
      {"object.utterance.aqua.interpretation.event_name",
       "\"U.tell_me_about_artist_x\""},
      {"object.utterance.aqua.interpretation.music.artist_entity.freebase_mid",
       "\"/m/01vs_v8\""},
      {"object.utterance.aqua.interpretation.music.artist_entity.annotated_"
       "span",
       "\"madonna\""},
      {"object.utterance.aqua.interpretation.music.artist_entity."
       "interpretation_number",
       "0"},
      {"object.utterance.aqua.interpretation.music.artist_entity.entity_number",
       "0"},
  };

  for (const auto& kv : key_values) {
    string result;
    EXPECT_TRUE(datamodel_->EvaluateExpression(kv.first, &result));
    EXPECT_EQ(kv.second, result) << kv.first;
  }

  // Long doubles may be truncated by the JSON library.
  string result;
  EXPECT_TRUE(datamodel_->EvaluateExpression(
      "object.utterance.aqua.interpretation.music.artist_entity.confidence_"
      "score",
      &result));
  double expected = 0;
  double actual = 1;
  CHECK(::absl::SimpleAtod("0.79515373706817627", &expected));
  EXPECT_TRUE(::absl::SimpleAtod(result, &actual));
  EXPECT_DOUBLE_EQ(expected, actual);
}

// Assign a JSON object, retrieve it, put the JSON formatted result as a string
// field in a new JSON object, assign, and retrieve it again.
TEST_F(LightWeightDatamodelTest, AssignJSONRecursively) {
  string result;

  const char kJSON[] = R"({"a":"1","b":{"c":"\"2\"","d":"3"},"e":{"f":"4"}})";
  EXPECT_TRUE(DeclareAndAssign("object1", kJSON));
  EXPECT_TRUE(datamodel_->EvaluateExpression("object1", &result));
  EXPECT_EQ(kJSON, result);

  Json::Value root;
  root["nested"] = Json::Value(result);
  EXPECT_TRUE(
      datamodel_->AssignExpression("object1", Json::FastWriter().write(root)));
  EXPECT_TRUE(datamodel_->EvaluateExpression("object1", &result));
  // Fast writer produces a newline.
  EXPECT_EQ(Json::FastWriter().write(root), result + "\n");

  // Nest once more.
  root["nested"] = Json::Value(result);
  EXPECT_TRUE(
      datamodel_->AssignExpression("object1", Json::FastWriter().write(root)));
  EXPECT_TRUE(datamodel_->EvaluateExpression("object1", &result));
  // Fast writer produces a newline.
  EXPECT_EQ(Json::FastWriter().write(root), result + "\n");
}

// Test that declared ancestor objects are created if they do not exists.
TEST_F(LightWeightDatamodelTest, DeclareAncestorsCreated) {
  string result;

  // Test declare then assignment to a missing nested object's field, i.e.,
  // when 'object' already exists but 'object.cow' does not.
  EXPECT_TRUE(datamodel_->Declare("object"));
  EXPECT_TRUE(datamodel_->AssignExpression("object", "{}"));

  EXPECT_TRUE(datamodel_->Declare("object.cow.moo"));
  EXPECT_TRUE(datamodel_->AssignString("object.cow.moo", "yes"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("object.cow", &result));
  EXPECT_EQ(R"JSON({"moo":"yes"})JSON", result);

  // Test when no ancestors on the path to the assignment location exists.
  EXPECT_TRUE(datamodel_->Declare("other.cow.meow"));
  EXPECT_TRUE(datamodel_->AssignString("other.cow.meow", "no"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("other", &result));
  EXPECT_EQ(R"JSON({"cow":{"meow":"no"}})JSON", result);

  // Test declaring an object field after an array field.
  EXPECT_TRUE(datamodel_->Declare("array[0].foo"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("array", &result));
  EXPECT_EQ(R"([{"foo":null}])", result);
}

// Test that declaration errors are detected.
TEST_F(LightWeightDatamodelTest, DeclareError) {
  // Duplicate declaration should return false.
  EXPECT_TRUE(datamodel_->Declare("foo"));
  EXPECT_FALSE(datamodel_->Declare("foo"));

  // Conflicting object type declarations should return false.
  // Declaring an object field after declaring an array location.
  EXPECT_TRUE(datamodel_->Declare("array[0]"));
  EXPECT_FALSE(datamodel_->Declare("array.foo"));
  EXPECT_FALSE(datamodel_->Declare("array[\"foo\"]"));
  // Declaring an array location after declaring an object field.
  EXPECT_TRUE(datamodel_->Declare("obj[\"foo\"]"));
  EXPECT_FALSE(datamodel_->Declare("obj[2]"));
  // Declaring a location after declaring a null ancestor.
  EXPECT_TRUE(datamodel_->Declare("null_id"));
  EXPECT_FALSE(datamodel_->Declare("null_id[1]"));
  EXPECT_FALSE(datamodel_->Declare("null_id[\"foo\"]"));
}

TEST_F(LightWeightDatamodelTest, EvaluateStringExpression) {
  string result;

  EXPECT_TRUE(datamodel_->EvaluateStringExpression("\"foo\"", &result));
  EXPECT_EQ("foo", result);

  // Promote integer to string.
  EXPECT_TRUE(datamodel_->EvaluateStringExpression("5", &result));
  EXPECT_EQ("5", result);

  // Promote double to string.
  EXPECT_TRUE(datamodel_->EvaluateStringExpression("5.8", &result));
  double result_d = 0;
  CHECK(::absl::SimpleAtod(result, &result_d));
  EXPECT_DOUBLE_EQ(5.8, result_d);

  // Promote object to string.
  EXPECT_TRUE(datamodel_->EvaluateStringExpression("{\"foo\":5}", &result));
  EXPECT_EQ("{\"foo\":5}", result);

  // Evaluate string variable.
  EXPECT_TRUE(DeclareAndAssign("foo", "\"string\""));
  EXPECT_TRUE(datamodel_->EvaluateStringExpression("foo", &result));
  EXPECT_EQ("string", result);

  // Evaluate singly quoted strings.
  EXPECT_TRUE(datamodel_->EvaluateStringExpression("'foo'", &result));
  EXPECT_EQ("foo", result);
  EXPECT_TRUE(datamodel_->EvaluateStringExpression("'f\"o\"o'", &result));
  EXPECT_EQ("f\"o\"o", result);
}

TEST_F(LightWeightDatamodelTest, PlusDoublesAndBoolean) {
  string result;
  double resultd = 0;

  // Test adding integer and boolean values.
  LOG(INFO) << "1.0 + true";
  EXPECT_TRUE(datamodel_->EvaluateExpression("1.0 + true", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(2.0, resultd);

  LOG(INFO) << "true + 1.0";
  EXPECT_TRUE(datamodel_->EvaluateExpression("true + 1.0", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(2.0, resultd);

  LOG(INFO) << "1.0 + false";
  EXPECT_TRUE(datamodel_->EvaluateExpression("1.0 + false", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(1.0, resultd);

  LOG(INFO) << "false + 1.0";
  EXPECT_TRUE(datamodel_->EvaluateExpression("false + 1.0", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(1.0, resultd);

  // Test adding locations.
  EXPECT_TRUE(DeclareAndAssign("green", "true"));
  EXPECT_TRUE(DeclareAndAssign("red", "false"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("green + 1.0", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(2.0, resultd);
  EXPECT_TRUE(datamodel_->EvaluateExpression("red + 1.0", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(1.0, resultd);
}

TEST_F(LightWeightDatamodelTest, ArithmeticIntegersAndBoolean) {
  string result;

  // Test operations on integer and boolean values.
  EXPECT_TRUE(datamodel_->EvaluateExpression("1 + true", &result));
  EXPECT_EQ("2", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("true - 1", &result));
  EXPECT_EQ("0", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("1 * false", &result));
  EXPECT_EQ("0", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("false / 1", &result));
  EXPECT_EQ("0", result);

  // Test operations on locations.
  EXPECT_TRUE(DeclareAndAssign("green", "true"));
  EXPECT_TRUE(DeclareAndAssign("red", "false"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("green + 1", &result));
  EXPECT_EQ("2", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("red + 1", &result));
  EXPECT_EQ("1", result);
}

TEST_F(LightWeightDatamodelTest, PlusBoolean) {
  string result;

  // Test adding boolean values.
  EXPECT_TRUE(datamodel_->EvaluateExpression("true + true", &result));
  EXPECT_EQ("2", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("true + false", &result));
  EXPECT_EQ("1", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("false + false", &result));
  EXPECT_EQ("0", result);

  // Test adding locations.
  EXPECT_TRUE(DeclareAndAssign("green", "true"));
  EXPECT_TRUE(DeclareAndAssign("red", "false"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("green + green", &result));
  EXPECT_EQ("2", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("red + green", &result));
  EXPECT_EQ("1", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("red + red", &result));
  EXPECT_EQ("0", result);
}

TEST_F(LightWeightDatamodelTest, PlusIntegers) {
  string result;

  // Test adding integer values.
  EXPECT_TRUE(datamodel_->EvaluateExpression("1 + 1", &result));
  EXPECT_EQ("2", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("6 + 9 + 5", &result));
  EXPECT_EQ("20", result);

  // Test adding locations.
  EXPECT_TRUE(DeclareAndAssign("foo", "6"));
  EXPECT_TRUE(DeclareAndAssign("bar", "4"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("foo + bar", &result));
  EXPECT_EQ("10", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("foo + 10 + bar", &result));
  EXPECT_EQ("20", result);
}

TEST_F(LightWeightDatamodelTest, PlusDoubles) {
  string result;
  double resultd = 0;

  // Test adding doubles.
  EXPECT_TRUE(datamodel_->EvaluateExpression("1.0 + 1.0", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(2, resultd);
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.5 + 0.6 + 0.9", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(2, resultd);

  // Test integer to double promotion.
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.6 + 5 + 0.9", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(6.5, resultd);

  // Test adding locations.
  EXPECT_TRUE(DeclareAndAssign("foo", "0.6"));
  EXPECT_TRUE(DeclareAndAssign("bar", "0.4"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("foo + bar", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(1, resultd);
  EXPECT_TRUE(datamodel_->EvaluateExpression("foo + 10 + bar", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(11, resultd);
}

TEST_F(LightWeightDatamodelTest, PlusStrings) {
  string result;

  // Test concatenating strings.
  EXPECT_TRUE(datamodel_->EvaluateExpression(R"("foo" + "bar")", &result));
  EXPECT_EQ(R"("foobar")", result);
  EXPECT_TRUE(
      datamodel_->EvaluateExpression(R"("foo" + "bar" + "2")", &result));
  EXPECT_EQ(R"("foobar2")", result);
  EXPECT_TRUE(
      datamodel_->EvaluateExpression(R"("the quick" + " brown fox")", &result));
  EXPECT_EQ(R"("the quick brown fox")", result);
  EXPECT_TRUE(
      datamodel_->EvaluateExpression(R"("1 + 2" + " == " + "3 - 0")", &result));
  EXPECT_EQ(R"("1 + 2 == 3 - 0")", result);

  // Test integer/double promotion.
  EXPECT_TRUE(datamodel_->EvaluateExpression(R"("foo" + 2 + "bar")", &result));
  EXPECT_EQ(R"("foo2bar")", result);
  EXPECT_TRUE(
      datamodel_->EvaluateExpression(R"(1.5 + ":foo" + 2 + "bar")", &result));
  EXPECT_EQ(R"("1.5:foo2bar")", result);

  // Test with locations.
  EXPECT_TRUE(DeclareAndAssign("foo", R"("some")"));
  EXPECT_TRUE(DeclareAndAssign("bar", R"("thing")"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("foo + bar", &result));
  EXPECT_EQ(R"("something")", result);
  EXPECT_TRUE(DeclareAndAssign("foo_d", "2.5"));
  EXPECT_TRUE(datamodel_->EvaluateExpression(
      R"(foo + 123 + bar + "_wow_" + foo_d)", &result));
  EXPECT_EQ(R"("some123thing_wow_2.5")", result);

  // Promote stored objects to string.
  EXPECT_TRUE(DeclareAndAssign("obj", R"({"bar": 2})"));
  EXPECT_TRUE(datamodel_->EvaluateExpression(R"("foo " + obj)", &result));
  EXPECT_EQ(R"("foo {\"bar\":2}")", result);
}

TEST_F(LightWeightDatamodelTest, PlusError) {
  string result;

  // Test missing operands.
  EXPECT_FALSE(datamodel_->EvaluateExpression(R"( + "bar")", &result));
  EXPECT_FALSE(datamodel_->EvaluateExpression(R"("foo" + "bar" +)", &result));
  EXPECT_FALSE(datamodel_->EvaluateExpression(R"("foo" + + "bar")", &result));
  EXPECT_FALSE(datamodel_->EvaluateExpression(R"("foo" + - /)", &result));
  EXPECT_FALSE(datamodel_->EvaluateExpression(R"(- + "foo")", &result));
}

TEST_F(LightWeightDatamodelTest, UnaryMinus) {
  string result;

  // Test plain unary minus.
  EXPECT_TRUE(datamodel_->EvaluateExpression("-5", &result));
  EXPECT_EQ("-5", result);

  // Test repeated unary minus.
  EXPECT_TRUE(datamodel_->EvaluateExpression("-----1", &result));
  EXPECT_EQ("-1", result);

  // Test unary minus in the presence of binary operator.
  EXPECT_TRUE(datamodel_->EvaluateExpression("2 + -2", &result));
  EXPECT_EQ("0", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("---2 + -2", &result));
  EXPECT_EQ("-4", result);

  // Test error cases.
  // Missing operand.
  EXPECT_FALSE(datamodel_->EvaluateExpression("-", &result));
  // Not a number.
  EXPECT_FALSE(datamodel_->EvaluateExpression("-\"foo\"", &result));
}

// Test that number only arithmetic operators do not work with string.
TEST_F(LightWeightDatamodelTest, ArithmeticErrors) {
  string result;

  EXPECT_FALSE(datamodel_->EvaluateExpression("-\"a\"", &result));
  for (const string& op : {"-", "*", "/"}) {
    EXPECT_FALSE(datamodel_->EvaluateExpression(
        absl::StrCat("\"a\"", op, "\"b\""), &result));
    EXPECT_FALSE(datamodel_->EvaluateExpression(
        absl::StrCat("5.6", op, "\"b\""), &result));
    EXPECT_FALSE(datamodel_->EvaluateExpression(
        absl::StrCat("5.2", op, "1", op, "\"b\""), &result));
    EXPECT_FALSE(datamodel_->EvaluateExpression(absl::StrCat("\"A\"", op, "2"),
                                                &result));
  }
}

TEST_F(LightWeightDatamodelTest, MinusNumbers) {
  string result;
  double resultd = 0;

  // Test integers.
  EXPECT_TRUE(datamodel_->EvaluateExpression("2 - 2", &result));
  EXPECT_EQ("0", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("2 - 2 - 10", &result));
  EXPECT_EQ("-10", result);

  // Test integers and boolean.
  EXPECT_TRUE(datamodel_->EvaluateExpression("2 - true", &result));
  EXPECT_EQ("1", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("false - 2 - 10", &result));
  EXPECT_EQ("-12", result);

  // Test doubles.
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.5 - 0.1", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(0.4, resultd);
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.5 - 0.1 - 0.5", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(-0.1, resultd);
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.5 - 0.1 - 1", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(-0.6, resultd);
  EXPECT_TRUE(datamodel_->EvaluateExpression("2 - 0.1 - 0.3", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(1.6, resultd);

  // Test doubles and boolean.
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.5 - 0.1 - true", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(-0.6, resultd);
  EXPECT_TRUE(datamodel_->EvaluateExpression("false - 4.0", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(-4.0, resultd);

  // Test with unary operator.
  EXPECT_TRUE(datamodel_->EvaluateExpression("-2 - -2", &result));
  EXPECT_EQ("0", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.5 - - 0.1", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(0.6, resultd);
  EXPECT_TRUE(datamodel_->EvaluateExpression("-0.5 - 0.1 - -1", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(0.4, resultd);
}

TEST_F(LightWeightDatamodelTest, MultiplyNumbers) {
  string result;
  double resultd = 0;

  // Test integers.
  EXPECT_TRUE(datamodel_->EvaluateExpression("2 * 2", &result));
  EXPECT_EQ("4", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("2 * 2 * 10", &result));
  EXPECT_EQ("40", result);

  // Test integers and boolean.
  EXPECT_TRUE(datamodel_->EvaluateExpression("2 * true", &result));
  EXPECT_EQ("2", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("2 * false * 10", &result));
  EXPECT_EQ("0", result);

  // Test doubles.
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.5 * 0.1", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(0.05, resultd);
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.5 * 0.1 * 0", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(0, resultd);

  // Test doubles and boolean.
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.5 * 0.1 * true", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(0.05, resultd);
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.5 * 0.1 * false", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(0, resultd);
}

TEST_F(LightWeightDatamodelTest, DivideNumbers) {
  string result;
  double resultd = 0;

  // Test integers.
  EXPECT_TRUE(datamodel_->EvaluateExpression("5 / 2", &result));
  EXPECT_EQ("2", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("0 / 2", &result));
  EXPECT_EQ("0", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("2 * 2 / 10", &result));
  EXPECT_EQ("0", result);

  // Test integers and boolean.
  EXPECT_TRUE(datamodel_->EvaluateExpression("5 / true", &result));
  EXPECT_EQ("5", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("false / 2", &result));
  EXPECT_EQ("0", result);

  // Test doubles.
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.5 / 0.1", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(5, resultd);
  EXPECT_TRUE(datamodel_->EvaluateExpression("0.5 / 0.1 / 2", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(2.5, resultd);

  // Test doubles and boolean.
  EXPECT_TRUE(datamodel_->EvaluateExpression("5.0 / true", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(5.0, resultd);
  EXPECT_TRUE(datamodel_->EvaluateExpression("false / 2.0", &result));
  EXPECT_TRUE(::absl::SimpleAtod(result, &resultd));
  EXPECT_DOUBLE_EQ(0.0, resultd);

  // Test divide by zero.
  EXPECT_FALSE(datamodel_->EvaluateExpression("0.5 / 0.0", &result));
  EXPECT_FALSE(datamodel_->EvaluateExpression("0 / 0", &result));
}

TEST_F(LightWeightDatamodelTest, ArithmeticPrecedence) {
  string result;

  EXPECT_TRUE(datamodel_->EvaluateExpression("5 / 2 + 1", &result));
  EXPECT_EQ("3", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("1 + 5 / 2", &result));
  EXPECT_EQ("3", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("2 - 2 * 0", &result));
  EXPECT_EQ("2", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("2 - 2 * 4 + 9 / 3", &result));
  EXPECT_EQ("-3", result);
}

TEST_F(LightWeightDatamodelTest, ArithmeticLogicalPrecedence) {
  string result;

  EXPECT_TRUE(datamodel_->EvaluateExpression("4 / 2 + true", &result));
  EXPECT_EQ("3", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("true + 5 * 2", &result));
  EXPECT_EQ("11", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("false * 1 + 1", &result));
  EXPECT_EQ("1", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("true && true + 2", &result));
  EXPECT_EQ("true", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("true || true + 1", &result));
  EXPECT_EQ("true", result);
}

TEST_F(LightWeightDatamodelTest, AssignAndEvalBooleanValues) {
  bool result;

  EXPECT_TRUE(DeclareAndAssign("foo", "true"));
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("foo", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(datamodel_->AssignExpression("foo", "\"abc\""));
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("foo", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(datamodel_->AssignExpression("foo", "5.6"));
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("foo", &result));
  EXPECT_TRUE(result);

  EXPECT_TRUE(datamodel_->AssignExpression("foo", "false"));
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("foo", &result));
  EXPECT_FALSE(result);
  EXPECT_TRUE(datamodel_->AssignExpression("foo", "0.0"));
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("foo", &result));
  EXPECT_FALSE(result);
  EXPECT_TRUE(datamodel_->AssignExpression("foo", "\"\""));
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("foo", &result));
  EXPECT_FALSE(result);
}

TEST_F(LightWeightDatamodelTest, StringComparisons) {
  bool result;

  // Test '=='.
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("abc" == "abc")", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("abc" == "bac")", &result));
  EXPECT_FALSE(result);

  // Test '!='.
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("abc" != "bac")", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("abc" != "abc")", &result));
  EXPECT_FALSE(result);

  // Test '<'.
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("abc" < "bac")", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("abc" < "abc")", &result));
  EXPECT_FALSE(result);

  // Test '<='.
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("abc" <= "bac")", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("abc" <= "abc")", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("bac" <= "abc")", &result));
  EXPECT_FALSE(result);

  // Test '>'.
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("bac" > "abc")", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("abc" > "abc")", &result));
  EXPECT_FALSE(result);

  // Test '>='.
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("abc" >= "bac")", &result));
  EXPECT_FALSE(result);
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("abc" >= "abc")", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression(R"("bac" >= "abc")", &result));
  EXPECT_TRUE(result);

  // Test error.
  EXPECT_FALSE(datamodel_->EvaluateBooleanExpression(R"("abc" >= 1)", &result));
  EXPECT_FALSE(
      datamodel_->EvaluateBooleanExpression(R"(2.0 == "bac")", &result));
}

TEST_F(LightWeightDatamodelTest, NumericComparisons) {
  bool result;

  // Test '=='.
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("0 == 0.0", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("1 == 1.0", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("4 == 2", &result));
  EXPECT_FALSE(result);
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("4 == 2.0", &result));
  EXPECT_FALSE(result);

  // Test '!='.
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("3 != 2", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("2.0 != 2.0", &result));
  EXPECT_FALSE(result);

  // Test '<'.
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("20 < 2000", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("1.0 < 1.0", &result));
  EXPECT_FALSE(result);

  // Test '<='.
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("1 <= 1.0", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("0.3 <= 3", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("20 <= 0.2", &result));
  EXPECT_FALSE(result);

  // Test '>'.
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("123 > 0.123", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("1.46 > 1.46", &result));
  EXPECT_FALSE(result);

  // Test '>='.
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("1.1 >= 1.24", &result));
  EXPECT_FALSE(result);
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("10 >= 9.99999", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression("1.00001 >= 1.00001", &result));
  EXPECT_TRUE(result);

  // Test error.
  EXPECT_FALSE(datamodel_->EvaluateBooleanExpression("abc >= 1", &result));
  EXPECT_FALSE(datamodel_->EvaluateBooleanExpression("2.0 == bac", &result));
}

TEST_F(LightWeightDatamodelTest, NullComparisons) {
  bool result;

  EXPECT_TRUE(DeclareAndAssign("foo", ""));

  const std::pair<string, bool> test_cases[] = {
      {"foo == null", true}, {"foo != null", false},   {"null != null", false},
      {"null != 123", true}, {"false == null", false},
  };

  for (const auto& test_case : test_cases) {
    result = !test_case.second;
    EXPECT_TRUE(datamodel_->EvaluateBooleanExpression(test_case.first, &result))
        << test_case.first;
    EXPECT_EQ(test_case.second, result) << test_case.first;
  }

  // Test error.
  EXPECT_FALSE(datamodel_->EvaluateBooleanExpression("abc >= null", &result));
  EXPECT_FALSE(datamodel_->EvaluateBooleanExpression("null < 1.4", &result));
}

TEST_F(LightWeightDatamodelTest, BooleanComparisons) {
  bool result;

  const std::pair<string, bool> test_cases[] = {
      {"false != false", false},
      {"false == false", true},
      {"true != true", false},
      {"true == true", true},
  };

  for (const auto& test_case : test_cases) {
    result = !test_case.second;
    EXPECT_TRUE(datamodel_->EvaluateBooleanExpression(test_case.first, &result))
        << test_case.first;
    EXPECT_EQ(test_case.second, result) << test_case.first;
  }

  // Test error.
  // No relational comparisons.
  EXPECT_FALSE(datamodel_->EvaluateBooleanExpression("false < true", &result));
  // Promotions disallowed.
  EXPECT_FALSE(datamodel_->EvaluateBooleanExpression("1 == true", &result));
  EXPECT_FALSE(datamodel_->EvaluateBooleanExpression("5 == 'foo'", &result));
}

// Test that the relational comparison operators (e.g. '>', '<=', etc) has a
// higher precedence than the equality comparison operators (e.g. '==', '!=').
TEST_F(LightWeightDatamodelTest, RelationalPrecedesEquality) {
  bool result;

  // Correct evaluation order:
  // $ true == -1 < 0
  // $ true == true
  // $ true
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("true == -1 < 0", &result));
  EXPECT_TRUE(result);
}

TEST_F(LightWeightDatamodelTest, MathRandomComputation) {
  bool result;

  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression("Math.random() >= 0", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression("Math.random() < 1", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression(
      "Math.random() + Math.random() < 2", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression(
      "Math.random() + Math.random() >= 0", &result));
  EXPECT_TRUE(result);
}

TEST_F(LightWeightDatamodelTest, InStateComputation) {
  bool result;

  datamodel_->SetRuntime(&runtime_);

  EXPECT_CALL(runtime_, IsActiveState(Eq("active_state")))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(runtime_, IsActiveState(Eq("inactive_state")))
      .WillRepeatedly(Return(false));

  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression("In('active_state')", &result));
  EXPECT_TRUE(result);

  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("In('active' + '_state')",
                                                    &result));
  EXPECT_TRUE(result);

  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression(
      "((In('active' + '_state') || false) && true)", &result));
  EXPECT_TRUE(result);

  EXPECT_TRUE(
      datamodel_->EvaluateBooleanExpression("In('inactive_state')", &result));
  EXPECT_FALSE(result);

  // Argument to In() must be a string.
  EXPECT_FALSE(datamodel_->EvaluateBooleanExpression("In(42)", &result));
}

// Test the boolean '!', '&&' and '||' operations.
TEST_F(LightWeightDatamodelTest, BooleanOperations) {
  bool result;

  // Truth table.
  const std::pair<string, bool> truth_table[] = {
      // NOT
      {"!0", true},
      {"!1", false},
      {"!!!!1", true},
      // AND
      {"0 && 0", false},
      {"0 && 1", false},
      {"1 && 0", false},
      {"1 && 1", true},
      // OR
      {"0 || 0", false},
      {"0 || 1", true},
      {"1 || 0", true},
      {"1 || 1", true},
      // Left to right associativity.
      {"1 && 1 && 0", false},
      {"1 && 1 && 1", true},
      {"0 || 0 || 1", true},
      {"0 || 0 || 0", false},
      // AND precedes OR.
      {"1 && 1 || 0 && 0", true},
      // Type casting.
      {"true && false", false},
      {"5.6 && 0.0", false},
      {"null || null", false},
      {"\"A\" && 2", true},
      {"!null", true},
      {"!true", false},
      // Complex boolean.
      {"!1 || 0 && 1", false},
      {"!0 && !0", true},
      {"!0 || !!!1", true},
  };
  for (const auto& entry : truth_table) {
    result = !entry.second;
    EXPECT_TRUE(datamodel_->EvaluateBooleanExpression(entry.first, &result));
    EXPECT_EQ(entry.second, result) << entry.first;
  }
}

// Test that the () parentheses works as expected.
TEST_F(LightWeightDatamodelTest, ParenthesesTest) {
  string result;

  const std::pair<string, string> success_test_cases[] = {
      // Base case.
      {"(0)", "0"},
      // Unary operators.
      {"-(-1)", "1"},
      // Single parenthesis.
      {"1 - (1 - 1)", "1"},
      {"2 * (1 + 2) * 2", "12"},
      // Multiple parentheses.
      {"(1 || 0) && (1 || 0)", "true"},
      {"2 * (1 + 2) - (2 / 2)", "5"},
      // Nested parentheses.
      {"1 - (1 - (1 - (1 - (1 - 1))))", "0"},
      {"((1 + 2) - (3 - 2)) * ((1 + 2) - (3 - 2))", "4"},
  };
  for (const auto& entry : success_test_cases) {
    result.clear();
    EXPECT_TRUE(datamodel_->EvaluateExpression(entry.first, &result));
    EXPECT_EQ(entry.second, result) << entry.first;
  }
  const string error_test_cases[] = {
      ")0(", "0(", ")0", "(1 + 2) * 2)", "(1) + (2", "1 - (1 - (1 - (1 - 1))",
  };
  for (const auto& entry : error_test_cases) {
    EXPECT_FALSE(datamodel_->EvaluateExpression(entry, &result)) << entry;
  }
}

// Test that element access works.
TEST_F(LightWeightDatamodelTest, ElementAccess) {
  string result;

  // Test 1d array access.
  EXPECT_TRUE(DeclareAndAssign("array1", "[0, 1, 2, 3]"));
  for (int i = 0; i < 4; ++i) {
    EXPECT_TRUE(datamodel_->EvaluateExpression(
        ::absl::Substitute("array1[$0]", i), &result));
    EXPECT_EQ(::absl::StrCat(i), result);
  }

  // Test 2d array access.
  EXPECT_TRUE(datamodel_->AssignExpression("array1",
                                           "[[0, 0], [1, 2], [2, 4], [3, 6]]"));
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 2; ++j) {
      EXPECT_TRUE(datamodel_->EvaluateExpression(
          ::absl::Substitute("array1[$0][$1]", i, j), &result));
      EXPECT_EQ(::absl::StrCat(i * (j + 1)), result);
    }
  }

  // Test flat object access.
  EXPECT_TRUE(DeclareAndAssign("obj1", R"({"foo":1,"bar":2})"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("obj1[\"foo\"]", &result));
  EXPECT_EQ("1", result);
  EXPECT_TRUE(datamodel_->EvaluateExpression("obj1[\"bar\"]", &result));
  EXPECT_EQ("2", result);

  // Test nested object access.
  EXPECT_TRUE(datamodel_->AssignExpression(
      "obj1", R"({"0":{"foo0":0},"1":{"foo1":1},"2":{"foo2":2}})"));
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(datamodel_->EvaluateExpression(
        ::absl::Substitute("obj1[\"$0\"][\"foo$1\"]", i, i), &result));
    EXPECT_EQ(::absl::StrCat(i), result);
  }

  // Test nested object access with subpaths instead of element access operator.
  EXPECT_TRUE(datamodel_->AssignExpression(
      "obj1", R"({"0":{"foo0":0},"1":{"foo1":1},"2":{"foo2":2}})"));
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(datamodel_->EvaluateExpression(
        ::absl::Substitute("obj1[\"$0\"].foo$1", i, i), &result));
    EXPECT_EQ(::absl::StrCat(i), result);
  }

  // Test array in object access.
  EXPECT_TRUE(datamodel_->AssignExpression(
      "obj1", R"({"0":{"foo0":[0]},"1":{"foo1":[1]},"2":{"foo2":[2]}})"));
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(datamodel_->EvaluateExpression(
        ::absl::Substitute("obj1[\"$0\"][\"foo$1\"][0]", i, i), &result));
    EXPECT_EQ(::absl::StrCat(i), result);
  }

  // Test object in array access.
  EXPECT_TRUE(
      datamodel_->AssignExpression("array1", R"([{"0":0}, {"1":1}, {"2":2}])"));
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(datamodel_->EvaluateExpression(
        ::absl::Substitute("array1[$0][\"$1\"]", i, i), &result));
    EXPECT_EQ(::absl::StrCat(i), result);
  }

  // Test with expression in [].
  EXPECT_TRUE(
      datamodel_->AssignExpression("array1", R"([{"0":0}, {"1":1}, {"2":2}])"));
  EXPECT_TRUE(
      datamodel_->EvaluateExpression("array1[1+1][\"\"+\"2\"]", &result));
  EXPECT_EQ("2", result);
  EXPECT_TRUE(datamodel_->AssignExpression(
      "obj1", R"({"0":{"foo0":0},"1":{"foo1":1},"2":{"foo2":2}})"));
  EXPECT_TRUE(datamodel_->EvaluateExpression(
      R"(array1[obj1["2"]["foo"+2]]["2"])", &result));
  EXPECT_EQ("2", result);

  // Test array out of bounds and missing object fields.
  EXPECT_TRUE(
      datamodel_->AssignExpression("array1", R"([{"0":0}, {"1":1}, {"2":2}])"));
  EXPECT_TRUE(datamodel_->AssignExpression(
      "obj1", R"({"0":{"foo0":0},"1":{"foo1":1},"2":{"foo2":2}})"));
  const string invalid_cases[] = {
      "array1[-5]",
      "array1[5]",
      "array1[0][\"5\"]",
      R"(obj1["2"]["foo"+3])",
  };
  for (const auto& invalid_case : invalid_cases) {
    EXPECT_FALSE(datamodel_->EvaluateExpression(invalid_case, &result));
    EXPECT_FALSE(datamodel_->IsDefined(invalid_case));
  }
}

TEST_F(LightWeightDatamodelTest, AssignWithLocationExpressions) {
  struct TestCase {
    // Path to root object or array.
    string object;
    // Expressions after the root.
    string subpath;
    // The value to assign.
    string value;
    // The entire object or array retrieved.
    string object_value;
  };

  const TestCase test_cases1[] = {
      // Test arrays.
      {"array1", "", "[]", "[]"},  // Initialize.
      {"array1", "[0]", "0", "[0]"},
      // Auto expand, old value at [0] should remain.
      {"array1", "[2]", "1", "[0,null,1]"},
      {"array1", "[1]", "2", "[0,2,1]"},
      // Nesting.
      {"array1", "[1]", "[0]", "[0,[0],1]"},
      {"array1", "[1][2]", "0", "[0,[0,null,0],1]"},
      // Expressions.
      {"array1", "[10-8]", "2", "[0,[0,null,0],2]"},
      // Test objects.
      {"obj1", "", "{}", "{}"},  // Initialize.
      {"obj1", R"(["foo"])", "0", R"({"foo":0})"},
      // Nesting.
      {"obj1", R"(["foo2"])", "{}", R"({"foo":0,"foo2":{}})"},
      {"obj1", R"(["foo2"]["bar"])", "1", R"({"foo":0,"foo2":{"bar":1}})"},
      {"obj1", R"(["foo2"]["bar2"])", "2",
       R"({"foo":0,"foo2":{"bar":1,"bar2":2}})"},
      // Trailing subpath.
      {"obj1", R"(["foo2"].bar)", "2",
       R"({"foo":0,"foo2":{"bar":2,"bar2":2}})"},
      // Expressions.
      {"obj1", R"(["fo"+"o2"])", "3", R"({"foo":0,"foo2":3})"},
      // Replace object with array.
      {"obj1", "", "[1]", "[1]"},
      // Replace array with object.
      {"obj1", "", R"({"foo":0})", R"({"foo":0})"},
      // Array in object.
      {"obj1", R"(["bar"])", "[]", R"({"bar":[],"foo":0})"},
      {"obj1", R"(["bar"][1])", "1", R"({"bar":[null,1],"foo":0})"},
      // Object in array.
      {"array1", "[0]", R"({"foo":6})", R"([{"foo":6},[0,null,0],2])"},
  };

  string result;
  for (const auto& test_case : test_cases1) {
    result.clear();
    string test_expr = absl::Substitute("Assign: $0$1 = $2", test_case.object,
                                        test_case.subpath, test_case.value);
    if (!datamodel_->IsDefined(test_case.object)) {
      EXPECT_TRUE(datamodel_->Declare(test_case.object)) << test_case.object;
    }
    EXPECT_TRUE(datamodel_->AssignExpression(
        test_case.object + test_case.subpath, test_case.value))
        << test_expr;
    EXPECT_TRUE(datamodel_->EvaluateExpression(
        test_case.object + test_case.subpath, &result))
        << test_expr;
    EXPECT_EQ(test_case.value, result) << test_expr;
    EXPECT_TRUE(datamodel_->EvaluateExpression(test_case.object, &result))
        << test_expr;
    EXPECT_EQ(test_case.object_value, result) << test_expr;
  }

  // Error cases.
  EXPECT_TRUE(DeclareAndAssign("err_array", "[0, 1, 3]"));
  EXPECT_TRUE(DeclareAndAssign("err_obj", R"({"foo":6})"));
  EXPECT_TRUE(DeclareAndAssign("err_null", "null"));
  // Attempt to assign to a non-array location with index element access.
  EXPECT_FALSE(datamodel_->AssignExpression("err_array[1][0]", "1"));
  EXPECT_FALSE(datamodel_->AssignExpression("err_obj[0]", "1"));
  // Attempt to assign to a non-object location with dictionary element access.
  EXPECT_FALSE(datamodel_->AssignExpression("err_array[\"foo\"]", "1"));
  EXPECT_FALSE(datamodel_->AssignExpression("err_obj[\"foo\"][\"bar\"]", "1"));
  // Attempt to assign element on a null value.
  EXPECT_FALSE(datamodel_->AssignExpression("err_null[1]", "1"));
  EXPECT_FALSE(datamodel_->AssignExpression("err_null[\"foo\"]", "1"));
}

// The result of a boolean 'not' system function call.
ACTION(ReturnBooleanNot) {
  if (arg1.size() != 1) {
    return false;
  }
  *arg2 = Json::Value(!arg1[0]->asBool());
  return true;
}

// The result of a boolean 'and' system function call.
ACTION(ReturnBooleanAnd) {
  if (arg1.size() != 2) {
    return false;
  }
  *arg2 = Json::Value(arg1[0]->asBool() && arg1[1]->asBool());
  return true;
}

ACTION(ReturnTestObject) {
  if (arg1.size() != 0) {
    return false;
  }
  Json::Value test_object;
  test_object["true_key"] = Json::Value(true);
  *arg2 = test_object;
  return true;
}

ACTION(ReturnTestArray) {
  if (arg1.size() != 0) {
    return false;
  }
  Json::Value test_array;
  test_array[0] = Json::Value(true);
  *arg2 = test_array;
  return true;
}

// Sets up the dispatcher with default function calls.
void SetupMockFunctions(MockFunctionDispatcher* dispatcher) {
  typedef std::vector<const Json::Value*> Values;
  const Json::Value true_value(true);
  const Json::Value false_value(false);

  // Create functions, ftrue(), ffalse(), not(bool), and and(bool, bool).
  ON_CALL(*dispatcher, HasFunction("ftrue")).WillByDefault(Return(true));
  ON_CALL(*dispatcher, Execute("ftrue", Values{}, _))
      .WillByDefault(DoAll(SetArgPointee<2>(true_value), Return(true)));

  ON_CALL(*dispatcher, HasFunction("ffalse")).WillByDefault(Return(true));
  ON_CALL(*dispatcher, Execute("ffalse", Values{}, _))
      .WillByDefault(DoAll(SetArgPointee<2>(false_value), Return(true)));

  ON_CALL(*dispatcher, HasFunction("not")).WillByDefault(Return(true));
  ON_CALL(*dispatcher, Execute("not", _, _)).WillByDefault(ReturnBooleanNot());

  ON_CALL(*dispatcher, HasFunction("and")).WillByDefault(Return(true));
  ON_CALL(*dispatcher, Execute("and", _, _)).WillByDefault(ReturnBooleanAnd());

  ON_CALL(*dispatcher, HasFunction("GetTestObject"))
      .WillByDefault(Return(true));
  ON_CALL(*dispatcher, Execute("GetTestObject", _, _))
      .WillByDefault(ReturnTestObject());

  ON_CALL(*dispatcher, HasFunction("GetTestArray")).WillByDefault(Return(true));
  ON_CALL(*dispatcher, Execute("GetTestArray", _, _))
      .WillByDefault(ReturnTestArray());
}

TEST_F(LightWeightDatamodelTest, SystemFunctionCallExpressions) {
  SetupMockFunctions(dispatcher_.get());
  const std::pair<string, bool> kTestCases[] = {
      {"ftrue()", true},
      {"ffalse()", false},
      {"not(ftrue())", false},
      {"not(ffalse())", true},
      {"and(true, true)", true},
      {"and(false, true)", false},
      {"and(not(ffalse()), not(ffalse()))", true},
      {"not(and(not(ffalse()), not(ffalse())))", false},
      {"and(not(ffalse()), and(ftrue(), ftrue()))", true},
  };

  bool result = false;
  for (const auto& entry : kTestCases) {
    result = !entry.second;
    EXPECT_TRUE(datamodel_->EvaluateBooleanExpression(entry.first, &result))
        << entry.first;
    EXPECT_EQ(entry.second, result) << entry.first;
  }
}

// Test system function calls mixed with other expressions.
TEST_F(LightWeightDatamodelTest, MixedSystemFunctionCallExpressions) {
  SetupMockFunctions(dispatcher_.get());
  const std::pair<string, bool> kTestCases[] = {
      {"ftrue() && 1", true},
      {"ffalse() && 0", false},
      {"not(0 || ftrue())", false},
      {"not(0 || ffalse())", true},
      {"!and(true, true)", false},
      {"and(!0, !0)", true},
      {"and(not(0 || 0), !!not(ffalse()))", true},
      {"1 && 1 && not(and(not(ffalse()), not(ffalse())))", false},
      {"and(not(ffalse()), and(ftrue(), ftrue() || 0 || ffalse())) || 0", true},
      {"0 || !!and(not(ffalse()), and(ftrue(), ftrue() || 0 || ffalse())) || 0",
       true},
      // Element access after function call.
      {"GetTestObject().true_key", true},
      {"GetTestArray()[0]", true},
  };

  bool result = false;
  for (const auto& entry : kTestCases) {
    result = !entry.second;
    EXPECT_TRUE(datamodel_->EvaluateBooleanExpression(entry.first, &result))
        << entry.first;
    EXPECT_EQ(entry.second, result) << entry.first;
  }
}

// Test that attempting to declare or assign to a system function name results
// in an error.
TEST_F(LightWeightDatamodelTest, InvalidSystemFunctionUse) {
  SetupMockFunctions(dispatcher_.get());
  ON_CALL(*dispatcher_, HasFunction("foo.bar")).WillByDefault(Return(true));

  EXPECT_FALSE(datamodel_->Declare("and"));
  EXPECT_FALSE(datamodel_->Declare("and[0]"));
  EXPECT_FALSE(datamodel_->Declare("not['foo']"));
  EXPECT_FALSE(datamodel_->Declare("not.foo.bar"));
  EXPECT_FALSE(datamodel_->Declare("foo.bar"));
  EXPECT_FALSE(datamodel_->Declare("foo.bar.far"));

  EXPECT_FALSE(datamodel_->AssignString("and", "hello"));
  EXPECT_FALSE(datamodel_->AssignString("and[0]", "hello"));
  EXPECT_FALSE(datamodel_->AssignString("not['foo']", "hello"));
  EXPECT_FALSE(datamodel_->AssignString("not.foo.bar", "hello"));
  EXPECT_FALSE(datamodel_->AssignString("foo.bar", "hello"));
  EXPECT_FALSE(datamodel_->AssignString("foo.bar.far", "hello"));
}

// The following documents a VALID case, i.e., that it is possible to
// create a location in the store using a dictionary that has the same name
// as a function. However it is only possible to access this store location
// using element access. In general, system function names take precedence over
// store location names.
TEST_F(LightWeightDatamodelTest, DictionaryDeclarationWithFunctionNameIsValid) {
  ON_CALL(*dispatcher_, HasFunction("foo.bar")).WillByDefault(Return(true));

  EXPECT_TRUE(datamodel_->Declare("foo['bar']"));
  EXPECT_TRUE(datamodel_->AssignExpression("foo['bar']", "1"));

  bool result = false;
  EXPECT_TRUE(datamodel_->EvaluateBooleanExpression("foo['bar']", &result));
  EXPECT_TRUE(result);

  // Cannot be accessed by a path name as that name is a system function and
  // they are not first class members, i.e., system functions are not values.
  EXPECT_FALSE(datamodel_->EvaluateBooleanExpression("foo.bar", &result));

  // Declaring fields beside the function is valid.
  EXPECT_TRUE(datamodel_->Declare("foo.other_bar"));

  // Reassigning a parent path name is valid.
  EXPECT_TRUE(datamodel_->AssignExpression("foo", "1"));
  // But the function at name 'foo.bar' will not be reassignable.
  EXPECT_FALSE(datamodel_->AssignExpression("foo.bar", "1"));
}

// Test that the array's 'length' property works. Unlike Javascript, 'length'
// is not assignable.
TEST_F(LightWeightDatamodelTest, ArrayLengthProperty) {
  string result;

  EXPECT_TRUE(DeclareAndAssign("myarray", "[]"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("myarray.length", &result));
  EXPECT_EQ("0", result);
  EXPECT_TRUE(datamodel_->AssignExpression("myarray[7]", ""));
  EXPECT_TRUE(datamodel_->EvaluateExpression("myarray.length", &result));
  EXPECT_EQ("8", result);

  // Access length property through element access on array is supported.
  result.clear();
  EXPECT_TRUE(datamodel_->EvaluateExpression("myarray['length']", &result));
  EXPECT_EQ("8", result);

  // Check the length of a nested array in array.
  EXPECT_TRUE(datamodel_->AssignExpression("myarray[0]", "[1,2]"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("myarray[0].length", &result));
  EXPECT_EQ("2", result);

  // Check the length of a nested array in object.
  EXPECT_TRUE(DeclareAndAssign("myobj_array", R"({"foo":[1,2,3]})"));
  EXPECT_TRUE(
      datamodel_->EvaluateExpression("myobj_array.foo.length", &result));
  EXPECT_EQ("3", result);
  result.clear();
  EXPECT_TRUE(
      datamodel_->EvaluateExpression("myobj_array['foo'].length", &result));
  EXPECT_EQ("3", result);

  // Assigning to the 'length' property is banned.
  EXPECT_FALSE(datamodel_->AssignExpression("myarray.length", "5"));
  EXPECT_FALSE(datamodel_->AssignExpression("myarray['length']", "5"));

  // Assigning to 'length' field of objects/dictionaries still work as expected.
  EXPECT_TRUE(datamodel_->AssignExpression("myobj_array.length", "2"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("myobj_array.length", &result));
  EXPECT_EQ("2", result);
  EXPECT_TRUE(datamodel_->AssignExpression("myobj_array['length']", "123"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("myobj_array['length']", &result));
  EXPECT_EQ("123", result);
}

TEST(LightWeightDatamodel, Clone) {
  MockFunctionDispatcher mock_dispatcher;
  std::unique_ptr<Datamodel> clone;
  {
    auto lwdm = LightWeightDatamodel::Create(&mock_dispatcher);
    CHECK(lwdm->Declare("key1"));
    CHECK(lwdm->AssignExpression("key1", R"("value")"));
    CHECK(lwdm->Declare("key2"));
    CHECK(lwdm->AssignExpression("key2", R"([ "value", { "foo" : "bar"} ])"));
    clone.reset(lwdm->Clone());
  }

  {
    string result;
    clone->EvaluateExpression("key1", &result);
    EXPECT_EQ(R"("value")", result);
  }
  {
    string result;
    clone->EvaluateExpression("key2", &result);
    EXPECT_EQ(R"(["value",{"foo":"bar"}])", result);
  }
}

TEST_F(LightWeightDatamodelTest, ArrayReferenceIteratorTest) {
  EXPECT_TRUE(DeclareAndAssign("myarray", "[0, 2, 4]"));
  auto iterator = WrapUnique(datamodel_->EvaluateIterator("myarray"));
  ASSERT_NE(nullptr, iterator);

  for (int i = 0; i < 3; ++i) {
    EXPECT_FALSE(iterator->AtEnd());
    EXPECT_EQ(::absl::StrCat(i), iterator->GetIndex());
    EXPECT_EQ(::absl::StrCat(i + i), iterator->GetValue());
    iterator->Next();
  }
  EXPECT_TRUE(iterator->AtEnd());
}

TEST_F(LightWeightDatamodelTest, ArrayValueIteratorTest) {
  auto iterator = WrapUnique(datamodel_->EvaluateIterator("[0, 2, 4]"));
  ASSERT_NE(nullptr, iterator);

  for (int i = 0; i < 3; ++i) {
    EXPECT_FALSE(iterator->AtEnd());
    EXPECT_EQ(::absl::StrCat(i), iterator->GetIndex());
    EXPECT_EQ(::absl::StrCat(i + i), iterator->GetValue());
    iterator->Next();
  }
  EXPECT_TRUE(iterator->AtEnd());
}

TEST_F(LightWeightDatamodelTest, ArrayIteratorErrorExpressionTest) {
  for (const auto& expr : {"1", "myarray", "null", "+"}) {
    auto iterator = WrapUnique(datamodel_->EvaluateIterator(expr));
    EXPECT_EQ(nullptr, iterator) << "Expression: " << expr;
  }
}

TEST_F(LightWeightDatamodelTest, SerializeAsString) {
  EXPECT_EQ("null\n", datamodel_->SerializeAsString());

  // Set something in the data model.
  EXPECT_TRUE(DeclareAndAssign("foo", "1"));
  EXPECT_EQ("{\"foo\":1}\n", datamodel_->SerializeAsString());

  // Set some other variable in the data model.
  EXPECT_TRUE(DeclareAndAssign("myarray", "[0, 2, 4]"));
  EXPECT_EQ("{\"foo\":1,\"myarray\":[0,2,4]}\n",
            datamodel_->SerializeAsString());
}

// Tests for verifying de-serializing of datamodel works.
TEST_F(LightWeightDatamodelTest, CreateLightWeightDatamodel) {
  NiceMock<MockFunctionDispatcher> mock_dispatcher;
  {
    std::unique_ptr<Datamodel> lwdm =
        LightWeightDatamodel::Create("", &mock_dispatcher);
    EXPECT_EQ(nullptr, lwdm);  // empty string is not a valid serialization.
  }

  {
    std::unique_ptr<Datamodel> lwdm =
        LightWeightDatamodel::Create("null\n", &mock_dispatcher);
    EXPECT_NE(nullptr, lwdm);
    // Just verify that the newly created datamodel works (i.e., can read/write
    // variables).
    EXPECT_TRUE(lwdm->Declare("foo"));
    EXPECT_TRUE(lwdm->AssignExpression("foo", R"("bar")"));
    string foo_value;
    EXPECT_TRUE(lwdm->EvaluateExpression("foo", &foo_value));
    EXPECT_EQ(R"("bar")", foo_value);
  }

  {
    // Set something in the data model.
    std::unique_ptr<Datamodel> lwdm =
        LightWeightDatamodel::Create("{\"foo\":1}\n", &mock_dispatcher);
    EXPECT_NE(nullptr, lwdm);

    string foo_value;
    EXPECT_TRUE(lwdm->EvaluateExpression("foo", &foo_value));
    EXPECT_EQ("1", foo_value);
  }

  {
    std::unique_ptr<Datamodel> lwdm = LightWeightDatamodel::Create(
        "{\"foo\":1,\"myarray\":[0,2,4]}\n", &mock_dispatcher);
    EXPECT_NE(nullptr, lwdm);

    string myarray_value;
    EXPECT_TRUE(lwdm->EvaluateExpression("myarray", &myarray_value));
    EXPECT_EQ("[0,2,4]", myarray_value);
  }
}

// Test that JSON objects can be used in assignment.  It generally works, but
// not if your string contains \'
TEST_F(LightWeightDatamodelTest, AssignJSONList) {
  string result;

  const char kJSON1[] =
      R"(["I can ", "handle lists ", "and 'unescaped' apostrophes"])";
  EXPECT_TRUE(DeclareAndAssign("synonyms", kJSON1));
  EXPECT_TRUE(datamodel_->IsDefined("synonyms"));
  EXPECT_TRUE(datamodel_->EvaluateExpression("synonyms[0]", &result));
  EXPECT_EQ(R"("I can ")", result);
  const char kJSON2[] = R"(["I can\'t ", "handle escaped apostrophes")";
  EXPECT_FALSE(DeclareAndAssign("synonyms", kJSON2));
}

}  // namespace
}  // namespace state_chart
