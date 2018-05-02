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

#include "statechart/internal/model_impl.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <list>
#include <queue>
#include <utility>

#include "absl/algorithm/container.h"
#include "statechart/internal/model/model.h"
#include "statechart/internal/runtime.h"
#include "statechart/logging.h"
#include "statechart/platform/logging.h"
#include "statechart/platform/map_util.h"
#include "statechart/proto/state_machine_context.pb.h"

using ::std::placeholders::_1;
using ::absl::c_all_of;
using ::absl::c_any_of;
using ::absl::c_copy;
using ::gtl::c_contains_some_of;

namespace state_chart {

namespace {

// Implementation of SCXML pseudo-code addAncestorStatesToEnter.
// Compute the ancestor states that need to be entered from the current 'state'
// limited to 'ancestor'.
// Returns false if error occurred.
bool AddAncestorStatesToEnter(
    const model::State* state, const model::State* ancestor,
    std::set<const model::State*>* states_to_enter,
    std::set<const model::State*>* states_for_default_entry);

// Implementation of SCXML pseudo-code addDescendantStatesToEnter.
// Returns false if error occurred.
bool AddDescendantStatesToEnter(
    const model::State* state, std::set<const model::State*>* states_to_enter,
    std::set<const model::State*>* states_for_default_entry);

// Find the index of a value in a container. The index is the order number
// in the container's natural order.
// Return the index or -1 if not found.
template <class Container>
int GetIndex(const Container& container,
             const typename Container::value_type& value) {
  auto it = std::find(container.begin(), container.end(), value);
  if (it == container.end()) {
    return -1;
  }
  return std::distance(container.begin(), it);
}

// Computes the list of ancestors (youngest first) of this state up to
// 'state_ancestor_limit'. Parameter 'state_ancestor_limit' may be nullptr in
// which case all states up to the the top-level are returned except 'root'.
// The results are appended to 'ancestors'.
// Note that this differs from the SCXML algorithm in that nullptr (root) is not
// included when 'state_ancestor_limit' is nullptr.
// Returns false if error occurred.
bool GetProperAncestors(const model::State* state,
                        const model::State* state_ancestor_limit,
                        std::vector<const model::State*>* ancestors) {
  RETURN_FALSE_IF(state == nullptr);
  RETURN_FALSE_IF(ancestors == nullptr);

  for (const model::State* parent = state->GetParent();
       parent != state_ancestor_limit; parent = parent->GetParent()) {
    ancestors->push_back(parent);
  }
  return true;
}

// Return true if 'state1' is a descendant of 'state2'.
bool IsDescendant(const model::State* state1, const model::State* state2) {
  // Root is not a descendant of anything.
  if (state1 == nullptr) {
    return false;
  }
  // Everything is descendant of root.
  if (state2 == nullptr) {
    return true;
  }
  if (state1 == state2) {
    return false;
  }
  // Scan for ancestor.
  for (auto parent = state1->GetParent(); parent != nullptr;
       parent = parent->GetParent()) {
    if (parent == state2) {
      return true;
    }
  }
  return false;
}

// Searches a list of states for an enabled transition. The 'event_tokens' may
// be nullptr, in which case a eventless transition will be found.
// Returns the enabled transition or nullptr if none exists.
const model::Transition* FindEnabledTransition(
    Runtime* runtime, const std::vector<const model::State*>& states,
    const string* event) {
  for (auto state : states) {
    for (auto transition : state->GetTransitions()) {
      // If eventless but transition has an event, skip.
      // OR
      // If eventful but the event does not match, skip.
      if ((event == nullptr && !transition->GetEvents().empty()) ||
          (event != nullptr &&
           !Model::EventMatches(*event, transition->GetEvents()))) {
        continue;
      }
      if (transition->EvaluateCondition(runtime)) {
        return transition;
      }
    }
  }
  return nullptr;
}

/* SCXML pseudo-code:
procedure addAncestorStatesToEnter(state, ancestor, statesToEnter,
                                   statesForDefaultEntry,
                                   defaultHistoryContent)
 for anc in getProperAncestors(state,ancestor):
   statesToEnter.add(anc)
   if isParallelState(anc):
     for child in getChildStates(anc):
       if not statesToEnter.some(lambda s: isDescendant(s,child)):
         addDescendantStatesToEnter(child, statesToEnter,
                                    statesForDefaultEntry,
                                    defaultHistoryContent)
*/
bool AddAncestorStatesToEnter(
    const model::State* state, const model::State* ancestor,
    std::set<const model::State*>* states_to_enter,
    std::set<const model::State*>* states_for_default_entry) {
  std::vector<const model::State*> proper_ancestors;
  RETURN_FALSE_IF(!GetProperAncestors(state, ancestor, &proper_ancestors));
  for (const model::State* state : proper_ancestors) {
    states_to_enter->insert(state);
    if (state->IsParallel()) {
      for (const model::State* child : state->GetChildren()) {
        if (!c_any_of(*states_to_enter, [child](const model::State* s) {
              return IsDescendant(s, child);
            })) {
          RETURN_FALSE_IF(!AddDescendantStatesToEnter(
              child, states_to_enter, states_for_default_entry));
        }
      }
    }
  }
  return true;
}

/* SCXML pseudo-code:
procedure addDescendantStatesToEnter(state, statesToEnter,
                                     statesForDefaultEntry,
                                     defaultHistoryContent):
  if isHistoryState(state):
    if historyValue[state.id]:
      for s in historyValue[state.id]:
        addDescendantStatesToEnter(s, statesToEnter, statesForDefaultEntry,
                                   defaultHistoryContent)
        addAncestorStatesToEnter(s, state.parent, statesToEnter,
                                 statesForDefaultEntry, defaultHistoryContent)
    else:
      defaultHistoryContent[state.parent.id] = state.transition.content
      for s in state.transition.target:
        addDescendantStatesToEnter(s, statesToEnter, statesForDefaultEntry,
                                   defaultHistoryContent)
        addAncestorStatesToEnter(s, state.parent, statesToEnter,
                                 statesForDefaultEntry, defaultHistoryContent)
  else:
    statesToEnter.add(state)
    if isCompoundState(state):
      statesForDefaultEntry.add(state)
      for s in getEffectiveTargetStates(state.initial.transition):
        addDescendantStatesToEnter(s, statesToEnter, statesForDefaultEntry,
                                   defaultHistoryContent)
        addAncestorStatesToEnter(s, state, statesToEnter, statesForDefaultEntry,
                                 defaultHistoryContent)
    else:
      if isParallelState(state):
        for child in getChildStates(state):
           if not statesToEnter.some(lambda s: isDescendant(s,child)):
             addDescendantStatesToEnter(child, statesToEnter,
                                        statesForDefaultEntry,
                                        defaultHistoryContent)
*/
bool AddDescendantStatesToEnter(
    const model::State* state, std::set<const model::State*>* states_to_enter,
    std::set<const model::State*>* states_for_default_entry) {
  RETURN_FALSE_IF(state == nullptr);
  // TODO(qplau): Implement history logic.
  states_to_enter->insert(state);
  if (state->IsCompound()) {
    states_for_default_entry->insert(state);
    RETURN_FALSE_IF_MSG(
        state->GetInitialTransition() == nullptr,
        "Compound state has no initial transition: " << state->id());
    for (const auto* target_state :
         state->GetInitialTransition()->GetTargetStates()) {
      RETURN_FALSE_IF(!AddDescendantStatesToEnter(target_state, states_to_enter,
                                                  states_for_default_entry));
      RETURN_FALSE_IF(!AddAncestorStatesToEnter(
          target_state, state, states_to_enter, states_for_default_entry));
    }
  } else if (state->IsParallel()) {
    for (const auto* child : state->GetChildren()) {
      // ∄ s ∈ states_to_enter s.t. IsDescendant(s, child)
      if (!c_any_of(*states_to_enter, [child](const model::State* s) {
            return IsDescendant(s, child);
          })) {
        RETURN_FALSE_IF(!AddDescendantStatesToEnter(child, states_to_enter,
                                                    states_for_default_entry));
      }
    }
  }
  return true;
}

// SCXML pseudo code:
//
// function findLCCA(stateList):
//    for anc in getProperAncestors(stateList.head(), null).filter(
//            isCompoundStateOrScxmlElement):
//        if stateList.tail().every(lambda s: isDescendant(s, anc)):
//            return anc
//
// Computes the least common compound ancestor (LCCA), the 'state' or 'scxml'
// element 's', such that 's' is a proper ancestor of all states on 'states'
// ('stateList' in pseudo-code) and no descendant of 's' has this property.
// Parameter 'states' must not be empty.
// Return the LCCA or nullptr if the LCCA is the root.
const model::State* FindLeastCommonCompoundAncestor(
    const std::vector<const model::State*>& states) {
  RETURN_NULL_IF(states.empty());

  // Do the filter step on proper ancestors of 'states.head'.
  std::vector<const model::State*> proper_ancestors;
  RETURN_NULL_IF(
      !GetProperAncestors(states.front(), nullptr, &proper_ancestors));

  // Holds the compound ancestors.
  std::vector<const model::State*> filtered_states;
  for (const auto& state : proper_ancestors) {
    if (state->IsCompound()) {
      filtered_states.push_back(state);
    }
  }

  for (const auto& ancestor : filtered_states) {
    // Must be descendant of 'states.tail'.
    if (std::all_of(++states.begin(), states.end(),
                    std::bind(&IsDescendant, _1, ancestor))) {
      return ancestor;
    }
  }
  // If nothing is found, return nullptr that represents the root node.
  return nullptr;
}

// Return the compound state (or root) such that:
// 1) All states that are exited or entered as a result of taking 'transition'
//    are descendants of it.
// 2) No descendant of it is also a common ancestor.
// SCXML pseudocode:
//
// function getTransitionDomain(t)
//  tstates = getTargetStates(t.target)
//  if not tstates
//      return t.source
//  elif t.type == "internal" and isCompoundState(t.source) and
//       tstates.every(lambda s: isDescendant(s,t.source)):
//      return t.source
//  else:
//      return findLCCA([t.source].append(tstates))
//
// Note that this function will return nullptr to represent the top level
// 'state', i.e., the state chart root containing top-level states.
const model::State* GetTransitionDomain(const model::Transition* transition) {
  const auto& target_states = transition->GetTargetStates();
  if (target_states.empty()) {
    return transition->GetSourceState();
  } else if (transition->GetSourceState() == nullptr) {
    // This case occurs when dealing with the top level (root) state chart's
    // initial transition.
    return nullptr;
  } else if (transition->IsInternal() &&
             transition->GetSourceState()->IsCompound() &&
             c_all_of(target_states, std::bind(&IsDescendant, _1,
                                               transition->GetSourceState()))) {
    return transition->GetSourceState();
  } else {
    std::vector<const model::State*> state_list{transition->GetSourceState()};
    c_copy(target_states, back_inserter(state_list));
    return FindLeastCommonCompoundAncestor(state_list);
  }
}

}  // namespace

ModelImpl::ModelImpl(
    const string& name, const model::Transition* initial_transition,
    const std::vector<const model::State*>& top_level_states,
    config::StateChart::Binding datamodel_binding,
    const model::ExecutableContent* datamodel,
    const std::vector<const model::ModelElement*>& model_elements)
    : name_(name),
      initial_transition_(initial_transition),
      top_level_states_(top_level_states),
      datamodel_binding_(datamodel_binding),
      datamodel_(datamodel),
      model_elements_(model_elements.begin(), model_elements.end()) {}

// virtual
bool ModelImpl::StateDocumentOrderLessThan(const model::State* state1,
                                           const model::State* state2) const {
  // Equality case.
  if (state1 == state2) {
    return false;
  }

  // Check if one state is ancestor of other.
  std::vector<const model::State*> path1;
  RETURN_FALSE_IF(!GetProperAncestors(state1, nullptr, &path1));
  // If state2 is an ancestor of state1, state1 comes after state2.
  if (absl::c_linear_search(path1, state2)) {
    return false;
  }

  std::vector<const model::State*> path2;
  RETURN_FALSE_IF(!GetProperAncestors(state2, nullptr, &path2));
  // If state1 is an ancestor of state2, state1 comes before state2.
  if (absl::c_linear_search(path2, state1)) {
    return true;
  }

  // Reverse the path to be from root to state and append the states to the end.
  std::reverse(path1.begin(), path1.end());
  path1.push_back(state1);
  std::reverse(path2.begin(), path2.end());
  path2.push_back(state2);

  // The first unmatched index. The common ancestor is at i - 1.
  unsigned int i = 0;
  while (path1[i] == path2[i]) {
    ++i;
  }
  RETURN_FALSE_IF(i >= path1.size());
  RETURN_FALSE_IF(i >= path2.size());

  // The children of the common ancestor should be used for comparing document
  // order.
  if (i == 0) {
    return GetIndex(top_level_states_, path1[i]) <
           GetIndex(top_level_states_, path2[i]);
  } else {
    return GetIndex(path1[i - 1]->GetChildren(), path1[i]) <
           GetIndex(path2[i - 1]->GetChildren(), path2[i]);
  }
}

// virtual
void ModelImpl::SortStatesByDocumentOrder(
    bool reverse, std::vector<const model::State*>* states) const {
  std::stable_sort(
      states->begin(), states->end(),
      [this, reverse](const model::State* state1, const model::State* state2) {
        return StateDocumentOrderLessThan(reverse ? state2 : state1,
                                          reverse ? state1 : state2);
      });
}

/* SCXML Pseudocode. This method implements both functions.

When event == nullptr implements:

function selectEventlessTransitions():
  enabledTransitions = new OrderedSet()
  atomicStates = configuration.toList()
      .filter(isAtomicState).sort(documentOrder)
  for state in atomicStates:
    loop: for s in [state].append(getProperAncestors(state, null)):
      for t in s.transition:
        if not t.event and conditionMatch(t):
          enabledTransitions.add(t)
          break loop
  enabledTransitions = removeConflictingTransitions(enabledTransitions)
  return enabledTransitions

Otherwise implements:

function selectTransitions(event):
  enabledTransitions = new OrderedSet()
  atomicStates = configuration.toList()
      .filter(isAtomicState).sort(documentOrder)
  for state in atomicStates:
    loop: for s in [state].append(getProperAncestors(state, null)):
      for t in s.transition:
        if t.event and nameMatch(t.event, event.name) and conditionMatch(t):
          enabledTransitions.add(t)
          break loop
  enabledTransitions = removeConflictingTransitions(enabledTransitions)
  return enabledTransitions
*/
std::vector<const model::Transition*> ModelImpl::SelectTransitions(
    Runtime* runtime, const string* event) const {
  RETURN_VALUE_IF_MSG(runtime == nullptr,
                      std::vector<const model::Transition*>(),
                      "Null Runtime given to SelectTransitions");
  const auto active_states = runtime->GetActiveStates();

  // Filter atomic states.
  std::vector<const model::State*> atomic_states;
  std::copy_if(active_states.begin(), active_states.end(),
               std::back_inserter(atomic_states),
               std::bind(&model::State::IsAtomic, _1));

  SortStatesByDocumentOrder(false, &atomic_states);

  std::vector<const model::Transition*> enabled_transitions;
  std::vector<const model::State*> path_to_root;
  for (auto state : atomic_states) {
    path_to_root = {state};
    RETURN_VALUE_IF_MSG(!GetProperAncestors(state, nullptr, &path_to_root),
                        std::vector<const model::Transition*>(),
                        "SelectTransitions failed in GetProperAncestors");
    const model::Transition* enabled_transition =
        FindEnabledTransition(runtime, path_to_root, event);

    if (enabled_transition != nullptr) {
      enabled_transitions.push_back(enabled_transition);
    }
  }

  return RemoveConflictingTransitions(runtime, enabled_transitions);
}

// Note that this differs from the SCXML pseudo-code's 'computeEntrySet' in that
// 'states_to_enter' is sorted in entry order.
// Note that the runtime is needed for the 'historyValue[state.id()]' global
// variable, if history is implemented in the future.
//
// Returns false if error occurred.
bool ModelImpl::ComputeEntrySet(
    const Runtime* runtime,
    const std::vector<const model::Transition*>& transitions,
    std::vector<const model::State*>* states_to_enter,
    std::set<const model::State*>* states_for_default_entry) const {
  RETURN_FALSE_IF(runtime == nullptr);
  RETURN_FALSE_IF(states_to_enter == nullptr);
  RETURN_FALSE_IF(states_for_default_entry == nullptr);

  std::set<const model::State*> states_to_enter_set;
  for (auto transition : transitions) {
    const auto& target_states = transition->GetTargetStates();
    states_to_enter_set.insert(target_states.begin(), target_states.end());
  }
  // Enter initial child states if compound states were entered.
  for (auto state : states_to_enter_set) {
    RETURN_FALSE_IF(!AddDescendantStatesToEnter(state, &states_to_enter_set,
                                                states_for_default_entry));
  }
  // Add ancestors when entering compound states.
  for (auto transition : transitions) {
    const auto* ancestor_state = GetTransitionDomain(transition);
    for (const auto& target_state : transition->GetTargetStates()) {
      RETURN_FALSE_IF(!AddAncestorStatesToEnter(target_state, ancestor_state,
                                                &states_to_enter_set,
                                                states_for_default_entry));
    }
  }
  states_to_enter->assign(states_to_enter_set.begin(),
                          states_to_enter_set.end());
  SortStatesByDocumentOrder(false, states_to_enter);
  return true;
}

// Note this differs from the SCXML algorithm in that the returned result is
// already sorted in exit order (i.e., reverse document order).
//
// override
std::vector<const model::State*> ModelImpl::ComputeExitSet(
    const Runtime* runtime,
    const std::vector<const model::Transition*>& transitions) const {
  std::set<const model::State*> states_to_exit_set;
  RETURN_VALUE_IF_MSG(runtime == nullptr, std::vector<const model::State*>(),
                      "Null Runtime given to ComputeExitSet");
  const auto active_states = runtime->GetActiveStates();

  for (auto transition : transitions) {
    // Note that if the transition target is empty, the transition source will
    // be the domain. This causes implicit self transitions (empty target) to
    // never leave the active state (self) and hence never call their 'onexit'
    // executable. However, if the transition target explicitly specifies self,
    // the active state (self) will be exited with 'onexit' called.
    const model::State* domain = GetTransitionDomain(transition);
    for (auto state : active_states) {
      // Check if the state is a descendant of some state in the domain.
      if (IsDescendant(state, domain)) {
        states_to_exit_set.insert(state);
      }
    }
  }

  std::vector<const model::State*> states_to_exit(states_to_exit_set.begin(),
                                             states_to_exit_set.end());
  // Reverse document order.
  SortStatesByDocumentOrder(true, &states_to_exit);
  return states_to_exit;
}

namespace {

bool IsInFinalStateHelper(const model::State* state,
                          const std::set<const model::State*>& active_states) {
  if (state->IsCompound()) {
    return c_any_of(state->GetChildren(),
                    [&active_states](const model::State* s) {
                      return s->IsFinal() && active_states.count(s) > 0;
                    });
  } else if (state->IsParallel()) {
    return c_all_of(state->GetChildren(),
                    std::bind(&IsInFinalStateHelper, std::placeholders::_1,
                              std::cref(active_states)));
  }
  return false;
}

}  // namespace

bool ModelImpl::IsInFinalState(const Runtime* runtime,
                               const model::State* state) const {
  return IsInFinalStateHelper(state, runtime->GetActiveStates());
}

/* SCXML Pseudo-code:
function removeConflictingTransitions(enabledTransitions):
  filteredTransitions = new OrderedSet()
  // toList sorts the transitions in the order of the states that selected them
  for t1 in enabledTransitions.toList():
    t1Preempted = false;
    transitionsToRemove = new OrderedSet()
    for t2 in filteredTransitions.toList():
      if computeExitSet([t1]).hasIntersection(computeExitSet([t2])):
        if isDescendant(t1.source, t2.source):
          transitionsToRemove.add(t2)
        else:
          t1Preempted = true
          break
    if not t1Preempted:
      for t3 in transitionsToRemove.toList():
        filteredTransitions.delete(t3)
      filteredTransitions.add(t1)
  return filteredTransitions
*/
std::vector<const model::Transition*> ModelImpl::RemoveConflictingTransitions(
    const Runtime* runtime,
    const std::vector<const model::Transition*>& transitions) const {
  std::list<const model::Transition*> filtered_transitions;
  std::set<const model::Transition*> transitions_to_remove;

  for (const auto* t1 : transitions) {
    bool t1_preempted = false;
    transitions_to_remove.clear();

    for (const auto* t2 : filtered_transitions) {
      if (c_contains_some_of(ComputeExitSet(runtime, {t1}),
                             ComputeExitSet(runtime, {t2}))) {
        if (IsDescendant(t1->GetSourceState(), t2->GetSourceState())) {
          transitions_to_remove.insert(t2);
        } else {
          t1_preempted = true;
          break;
        }
      }
    }
    if (!t1_preempted) {
      // Order of removal should not matter here.
      for (auto it = filtered_transitions.begin(),
                it_end = filtered_transitions.end();
           it != it_end;
           /*empty*/) {
        if (transitions_to_remove.count(*it) > 0) {
          it = filtered_transitions.erase(it);
        } else {
          ++it;
        }
      }
      filtered_transitions.push_back(t1);
    }
  }
  return std::vector<const model::Transition*>(filtered_transitions.begin(),
                                          filtered_transitions.end());
}

namespace {

typedef std::pair<const StateMachineContext::Runtime::ActiveStateElement*,
             const model::State*>
    ActiveStatePair;

std::vector<ActiveStatePair> GetActiveStatePairs(
    const std::vector<const model::State*>& states,
    const proto2::RepeatedPtrField<
        StateMachineContext::Runtime::ActiveStateElement>& active_states) {
  std::vector<ActiveStatePair> result;
  for (const auto& active_state : active_states) {
    const string& active_state_id = active_state.id();
    const std::vector<const model::State*>::const_iterator& found_state_itr =
        ::absl::c_find_if(states, [&active_state_id](const model::State* s) {
          return s->id() == active_state_id;
        });
    if (found_state_itr == states.end()) {
      LOG(INFO) << "State [" << active_state_id << "] was not found";
    } else {
      result.emplace_back(&active_state, *found_state_itr);
    }
  }
  return result;
}

}  // namespace

// Returns pointers to state(s) for a given tree of active states.
std::vector<const model::State*> ModelImpl::GetActiveStates(
    const proto2::RepeatedPtrField<
        StateMachineContext::Runtime::ActiveStateElement>& active_states)
    const {
  std::vector<const model::State*> states;

  std::queue<ActiveStatePair> bfs_queue;
  // Initialize the queue.
  const std::vector<ActiveStatePair> top_level_active_state_pairs =
      GetActiveStatePairs(GetTopLevelStates(), active_states);
  for (const auto& pair : top_level_active_state_pairs) {
    bfs_queue.push(pair);
  }

  while (!bfs_queue.empty()) {
    const ActiveStatePair pair = bfs_queue.front();
    bfs_queue.pop();
    states.push_back(pair.second);
    if (!pair.first->active_child().empty()) {
      std::vector<ActiveStatePair> child_pairs = GetActiveStatePairs(
          pair.second->GetChildren(), pair.first->active_child());
      for (const auto& child_pair : child_pairs) {
        bfs_queue.push(child_pair);
      }
    }
  }
  return states;
}

}  // namespace state_chart
