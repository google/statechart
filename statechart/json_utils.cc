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

#include "statechart/json_utils.h"

#include <string>

#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/substitute.h"
#include "include/json/features.h"
#include "include/json/reader.h"
#include "include/json/value.h"
#include "include/json/writer.h"
#include "statechart/platform/protobuf.h"
#include "statechart/platform/types.h"
#include "statechart/platform/map_util.h"
#include "statechart/proto/state_machine_context.pb.h"

namespace state_chart {
namespace json_utils {

namespace {

using gtl::LazyStaticPtr;
using proto2::FieldDescriptor;
using proto2::TextFormat;

// A TextFormat::Printer which is initialized with correct FieldValuePrinters to
// print JSON values.
class JSONStyledPrinter : public TextFormat::Printer {
 public:
  class JSONFieldValuePrinter : public TextFormat::FieldValuePrinter {
   public:
    string PrintString(const string& json_string) const override;
  };

  JSONStyledPrinter() : TextFormat::Printer() {
    SetExpandAny(true);
    const FieldDescriptor* datamodel_field_desc =
        ::state_chart::StateMachineContext::descriptor()->FindFieldByName(
            "datamodel");

    auto json_field_value_printer =
        ::absl::make_unique<JSONFieldValuePrinter>();
    if (RegisterFieldValuePrinter(datamodel_field_desc,
                                  json_field_value_printer.get())) {
      // If registered successfully, 'json_field_value_printer' will be owned
      // by the printer.
      json_field_value_printer.release();
    }
  }
};

// override
string JSONStyledPrinter::JSONFieldValuePrinter::PrintString(
    const string& json_string) const {
  Json::Reader reader(Json::Features::strictMode());
  Json::Value value;
  const bool success =
      reader.parse(json_string, value, false /* collectComments */);
  if (!success) {
    // probably is not even JSON.
    return TextFormat::FieldValuePrinter::PrintString(json_string);
  }
  Json::StyledWriter writer;
  string result = writer.write(value);
  absl::StripTrailingAsciiWhitespace(&result);
  return absl::StrCat("#-- JSON --# ", result, " #-- JSON --#");
}

const TextFormat::Printer& GetJsonPrinter() {
  static LazyStaticPtr<JSONStyledPrinter> json_printer;
  return *json_printer;
}

}  // namespace

string DebugString(const proto2::Message& message) {
  string output;
  GetJsonPrinter().PrintToString(message, &output);
  return output;
}

}  // namespace json_utils
}  // namespace state_chart
