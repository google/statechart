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

#include "statechart/internal/utility.h"

#include <algorithm>
#include <bitset>
#include <utility>
#include <vector>

#include "absl/strings/escaping.h"
#include "absl/strings/escaping.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/strings/substitute.h"
#include "re2/re2.h"
#include "statechart/platform/str_util.h"

namespace state_chart {

bool IsQuotedString(const string& str, const char quote_mark) {
  if (str.size() < 2 || str.front() != quote_mark || str.back() != quote_mark) {
    return false;
  }
  for (unsigned int i = 1; i < str.size() - 1; ++i) {
    if (str[i] == quote_mark && str[i - 1] != '\\') {
      return false;
    }
  }
  return true;
}

string Unquote(const string& str, const char quote_mark) {
  if (!IsQuotedString(str, quote_mark)) {
    return str;
  }
  // Unescape nested quotes.
  string result;
  strings::CharSet charset("\\");
  charset.Add(quote_mark);
  strings::BackslashUnescape(absl::string_view(str.c_str() + 1, str.size() - 2),
                             charset, &result);
  return result;
}

string Quote(const string& str) {
  if (IsQuotedString(str)) {
    return str;
  }
  // Escape nested quotes.
  return absl::StrCat("\"", EscapeQuotes(str), "\"");
}

string EscapeQuotes(const string& str) {
  string result;
  strings::BackslashEscape(str, strings::CharSet(R"(\")"), &result);
  return result;
}

bool MaybeJSON(const string& str) {
  // Ensure that JSON strings with newlines between braces will pass the match.
  static const LazyRE2 kJSONPattern = {R"RE((?s)\s*\{.*\}\s*)RE"};
  return RE2::FullMatch(str, *kJSONPattern);
}

bool MaybeJSONArray(const string& str) {
  // Ensure that JSON arrays with newlines between braces will pass the match.
  static const LazyRE2 kJSONPattern = {R"RE((?s)\s*\[.*\]\s*)RE"};
  return RE2::FullMatch(str, *kJSONPattern);
}

string MakeJSONError(const string& error_message) {
  return absl::StrCat("{\"error\": \"", EscapeQuotes(error_message).c_str(),
                      "\"}");
}

string MakeJSONFromStringMap(const std::map<string, string>& data_map) {
  std::vector<string> json_pair;
  for (const auto& data : data_map) {
    json_pair.push_back(absl::Substitute(
        "\"$0\":$1", EscapeQuotes(data.first).c_str(), data.second.c_str()));
  }
  return absl::StrCat("{", absl::StrJoin(json_pair, ","), "}");
}

}  // namespace state_chart
