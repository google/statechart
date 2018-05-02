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

#include "statechart/internal/model.h"

#include "absl/strings/match.h"

namespace state_chart {

// static
bool Model::EventMatches(const string& event_name,
                         const std::vector<string>& events) {
  // Search the list of event names.
  for (const string& transition_event : events) {
    // Wildcard matches all.
    if (transition_event == "*") {
      return true;
    }
    // The transition_event must be a prefix of event in the event hierarchy.
    // For example, the transition events, 'event_A' and 'event_A.sub_event_B',
    // will match the event, 'event_A.sub_event_B.something_else'.
    if (absl::StartsWith(event_name, transition_event) &&
        (event_name.size() == transition_event.size() ||
         event_name[transition_event.size()] == '.')) {
      return true;
    }
  }  // for event_to_match
  return false;
}

}  // namespace state_chart
