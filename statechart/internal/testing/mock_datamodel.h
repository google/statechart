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

#ifndef STATE_CHART_INTERNAL_TESTING_MOCK_DATAMODEL_H_
#define STATE_CHART_INTERNAL_TESTING_MOCK_DATAMODEL_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <glog/logging.h>
#include <gmock/gmock.h>

#include "absl/strings/str_cat.h"
#include "statechart/internal/datamodel.h"
#include "statechart/internal/runtime.h"
#include "statechart/logging.h"

ACTION_P(ReturnEvaluationResult, val) {
  *arg1 = val;
  return true;
}

ACTION(ReturnEvaluationError) { return false; }

namespace state_chart {

// A fake iterator that iterates through a vector of strings
class MockIterator : public Iterator {
 public:
  explicit MockIterator(const std::vector<string>& collection)
      : index_(0), collection_(collection) {}

  bool AtEnd() const override { return index_ >= collection_.size(); }

  bool Next() override {
    RETURN_FALSE_IF(AtEnd());
    ++index_;
    return true;
  }

  string GetValue() const override {
    RETURN_VALUE_IF_MSG(AtEnd(), "", "AtEnd() is true.");
    return collection_[index_];
  }

  string GetIndex() const override { return ::absl::StrCat(index_); }

 private:
  unsigned int index_;
  const std::vector<string> collection_;
};

class MockDatamodel : public Datamodel {
 public:
  MockDatamodel() {}
  MockDatamodel(const MockDatamodel&) = delete;
  MockDatamodel& operator=(const MockDatamodel&) = delete;
  virtual ~MockDatamodel() {}

  MOCK_CONST_METHOD1(IsDefined, bool(const string&));
  MOCK_METHOD1(Declare, bool(const string&));
  MOCK_METHOD2(AssignExpression, bool(const string&, const string&));
  MOCK_METHOD2(AssignString, bool(const string&, const string&));
  MOCK_CONST_METHOD2(EvaluateBooleanExpression, bool(const string&, bool*));
  MOCK_CONST_METHOD2(EvaluateStringExpression, bool(const string&, string*));
  MOCK_CONST_METHOD2(EvaluateExpression, bool(const string&, string*));
  MOCK_CONST_METHOD1(EncodeParameters, string(const std::map<string, string>&));
  MOCK_CONST_METHOD0(DebugString, string());
  MOCK_METHOD0(Clear, void());
  MOCK_CONST_METHOD0(Clone, std::unique_ptr<Datamodel>());
  MOCK_CONST_METHOD0(SerializeAsString, string());
  MOCK_CONST_METHOD1(EvaluateIterator,
                     std::unique_ptr<Iterator>(const string&));
  MOCK_METHOD1(ParseFromString, bool(const string&));
  MOCK_CONST_METHOD0(GetRuntime, const Runtime*());
  MOCK_METHOD1(SetRuntime, void(const Runtime*));
};

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_TESTING_MOCK_DATAMODEL_H_
