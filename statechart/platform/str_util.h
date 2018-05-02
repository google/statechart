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

#ifndef STATECHART_PLATFORM_STR_UTIL_H_
#define STATECHART_PLATFORM_STR_UTIL_H_

#include <bitset>

#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "absl/strings/strip.h"
#include "absl/strings/numbers.h"
#include "statechart/platform/types.h"

namespace strings {

class CharSet {
 public:
  explicit CharSet(absl::string_view characters);

  // Add or remove a character from the set.
  void Add(unsigned char c) { bits_.set(c); }
  void Remove(unsigned char c) { bits_.reset(c); }

  // Return true if this character is in the set
  bool Test(unsigned char c) const { return bits_[c]; }

 private:
  std::bitset<256> bits_;
};

void BackslashUnescape(absl::string_view src, const CharSet& to_unescape,
                       string* dest);

void BackslashEscape(absl::string_view src, const CharSet& to_escape,
                     string* dest);

inline string StripSuffixString(absl::string_view str,
                                absl::string_view suffix) {
  return string(absl::StripSuffix(str, suffix));
}

}  // namespace strings

#endif  // STATECHART_PLATFORM_STR_UTIL_H_
