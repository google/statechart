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

// A collection of utilities.
#ifndef STATE_CHART_INTERNAL_UTILITY_H_
#define STATE_CHART_INTERNAL_UTILITY_H_

#include <algorithm>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "statechart/platform/types.h"

namespace state_chart {

// Detect string has quotes i.e., the c-string is "\"quoted\"". Parameter
// 'quote_mark' can be used to change the quote character (e.g., '"' or '\'').
bool IsQuotedString(const string& str, const char quote_mark);

inline bool IsQuotedString(const string& str) {
  return IsQuotedString(str, '"');
}

// Detect if a string is surrounded by '{' and '}'.
bool MaybeJSON(const string& str);

// Detect if a string is surrounded by '[' and ']'.
bool MaybeJSONArray(const string& str);

// Returns an unquoted quoted string (e.g., "quoted" from "\"quoted\""), or
// the given string if the string is unquoted. Parameter 'quote_mark' can be
// used to change the quote character (e.g., '"' or '\''). Nested quotes within
// the string will be unescaped once.
string Unquote(const string& str, const char quote_mark);

inline string Unquote(const string& str) { return Unquote(str, '"'); }

// Returns a doubly quoted string (e.g., "unquoted" becomes "\"unquoted\"") if
// the given string is unquoted, or the original string if it is doubly quoted.
// Nested double quotes with the string will be escaped with backslash.
string Quote(const string& str);

// Return a string representing a JSON string with all double quotes escaped
// with backslash.
string EscapeQuotes(const string& str);

// Create a JSON event payload for an error message.
string MakeJSONError(const string& error_message);

// Create a JSON string with keys and values provided by a data map.
// Keys will be double quoted while values will not.
string MakeJSONFromStringMap(const std::map<string, string>& data_map);

// Search for a sub-sequence in the range [first, last) such that each element
// in the sub-sequence has its corresponding predicate evaluated to true.
// 'Predicate' should take in a single element argument and return a bool.
// Return the iterator to the first element in the first matching subsequence,
// otherwise return 'last' when there are no matches.
// This is similar to std::search except that a custom predicate is provided
// for each sub-sequence element.
template <class InputIterator, class Predicate>
InputIterator SearchWithPredicates(
    InputIterator first, InputIterator last,
    const std::vector<Predicate>& predicates);

// Search for the first occurrence of the outer-most parentheses within the
// range [first, last) where StartMatcher returns true when matching the
// parentheses start and EndMatcher returns true when matching the parentheses
// end. Caller ensures that 'start_match' and 'end_match' must each return the
// same result when called on the same element more than once in the range.
// Returns the pair of iterators to the start and end of the first
// occurrence of an outer-most parenthesis. If no parenthesis is found, or if
// first occurrence of a start match does not have a matching end match,
// return a pair of 'last' iterators.
// Note that this method does not validate if all parentheses are closed in the
// sequence. Hence an input of "(()" will result in no parenthesis found while
// an input of "())" will result in the first "()" being returned.
template <class InputIterator, class StartMatcher, class EndMatcher>
std::pair<InputIterator, InputIterator> FindOuterMostParentheses(
    InputIterator first, InputIterator last, StartMatcher start_match,
    EndMatcher end_match);

//------------------------------------------------------------------------------
// Implementation.

template <class InputIterator, class Predicate>
InputIterator SearchWithPredicates(InputIterator first, InputIterator last,
                                   const std::vector<Predicate>& predicates) {
  for (; first != last; ++first) {
    InputIterator it = first;
    bool success = true;
    for (const auto& predicate : predicates) {
      // The predicate sequence is longer than the remainder, there is no match.
      if (it == last) {
        return last;
      }
      if (predicate(*it)) {
        ++it;
      } else {
        success = false;
        break;
      }
    }
    if (success) {
      return first;
    }
  }
  return last;
}

template <class InputIterator, class StartMatcher, class EndMatcher>
std::pair<InputIterator, InputIterator> FindOuterMostParentheses(
    InputIterator first, InputIterator last, StartMatcher start_match,
    EndMatcher end_match) {
  if (std::distance(first, last) < 2) {
    return std::make_pair(last, last);
  }
  auto start_or_end_match = [&start_match, &end_match](
      const typename InputIterator::value_type& x) {
    return start_match(x) || end_match(x);
  };
  auto start = last;
  int count = 0;
  auto it = first;
  while (last != (it = std::find_if(it, last, start_or_end_match))) {
    if (start_match(*it)) {
      if (start == last) {
        start = it;
      }
      ++count;
    } else if (end_match(*it)) {
      --count;
      if (count == 0) {
        return std::make_pair(start, it);
      } else if (count <  0) {
        return std::make_pair(last, last);
      }
    }
    ++it;
  }
  return std::make_pair(last, last);
}

}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_UTILITY_H_
