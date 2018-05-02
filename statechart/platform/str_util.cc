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

#include "statechart/platform/str_util.h"

namespace strings {

CharSet::CharSet(absl::string_view characters) {
  for (const char& c : characters) {
    Add(c);
  }
}

void BackslashUnescape(absl::string_view src,
                       const CharSet& to_unescape, string* dest) {
  typedef absl::string_view::const_iterator Iter;
  Iter first = src.begin();
  Iter last = src.end();
  bool escaped = false;
  for (; first != last; ++first) {
    if (escaped) {
      if (to_unescape.Test(*first)) {
        dest->push_back(*first);
        escaped = false;
      } else {
        dest->push_back('\\');
        if (*first == '\\') {
          escaped = true;
        } else {
          escaped = false;
          dest->push_back(*first);
        }
      }
    } else {
      if (*first == '\\') {
        escaped = true;
      } else {
        dest->push_back(*first);
      }
    }
  }
  if (escaped) {
    dest->push_back('\\');  // trailing backslash
  }
}

void BackslashEscape(absl::string_view src, const CharSet& to_escape,
                     string* dest) {
  typedef absl::string_view::const_iterator Iter;
  Iter first = src.begin();
  Iter last = src.end();
  while (first != last) {
    // Advance to next character we need to escape, or to end of source
    Iter next = first;
    while (next != last && !to_escape.Test(*next)) {
      ++next;
    }
    // Append the whole run of non-escaped chars
    dest->append(first, next);
    if (next != last) {
      // Char at *next needs to be escaped.
      char c[2] = {'\\', *next++};
      dest->append(c, c + ABSL_ARRAYSIZE(c));
    }
    first = next;
  }
}

}  // namespace strings
