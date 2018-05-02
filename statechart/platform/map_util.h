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

#ifndef STATECHART_PLATFORM_MAP_UTIL_H_
#define STATECHART_PLATFORM_MAP_UTIL_H_

#include <algorithm>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include <glog/logging.h>

#include "absl/synchronization/mutex.h"

namespace gtl {

template <typename Map>
bool InsertIfNotPresent(Map* m, const typename Map::value_type::first_type& key,
                        const typename Map::value_type::second_type& value) {
  return m->insert({key, value}).second;
}

template <typename Set>
bool InsertIfNotPresent(Set* m, const typename Set::value_type& value) {
  return m->insert(value).second;
}

template <typename Collection>
void InsertOrDie(Collection* c, const typename Collection::value_type& value) {
  CHECK(InsertIfNotPresent(c, value)) << "duplicate value: " << value;
}

template <typename Map>
void InsertOrDie(Map* m, const typename Map::value_type::first_type& key,
                 const typename Map::value_type::second_type& value) {
  CHECK(InsertIfNotPresent(m, {key, value})) << "duplicate value: " << value;
}

template <typename Map>
bool InsertOrUpdate(Map* m, const typename Map::value_type::first_type& key,
                    const typename Map::value_type::second_type& value) {
  auto ret = m->insert({key, value});
  if (ret.second) return true;
  ret.first->second = value;  // update
  return false;
}

template <typename Container1, typename Container2>
bool c_contains_some_of(const Container1& container1,
                        const Container2& container2) {
  const auto& container1_last = std::end(container1);
  return std::find_first_of(std::begin(container1), container1_last,
                            std::begin(container2),
                            std::end(container2)) != container1_last;
}

template<typename ForwardIterator>
void STLDeleteContainerPointers(ForwardIterator begin, ForwardIterator end) {
  while (begin != end) {
    auto temp = begin;
    ++begin;
    delete *temp;
  }
}

template<typename T>
void STLDeleteElements(T* container) {
  if (!container) return;
  STLDeleteContainerPointers(container->begin(), container->end());
  container->clear();
}

template <typename Map, typename Key>
bool ContainsKey(const Map& map, const Key& key) {
  return map.find(key) != map.end();
}

template <typename Map>
const typename Map::value_type::second_type* FindOrNull(
    const Map& m, const typename Map::value_type::first_type& key) {
  auto it = m.find(key);
  if (it == m.end()) return nullptr;
  return &it->second;
}

// Helper type to mark missing c-tor argument types
// for Type's c-tor in LazyStaticPtr<Type, ...>.
struct NoArg {};

template <typename Type, typename Arg1 = NoArg, typename Arg2 = NoArg,
          typename Arg3 = NoArg>
class LazyStaticPtr {
 public:
  typedef Type element_type;  // per smart pointer convention

  // Pretend to be a pointer to Type (never NULL due to on-demand creation):
  Type &operator*() const { return *get(); }
  Type *operator->() const { return get(); }

  // Named accessor/initializer:
  Type *get() const {
    ::absl::MutexLock l(&mu_);
    if (!ptr_) Initialize(this);
    return ptr_;
  }

 public:
  // All the data is public and LazyStaticPtr has no constructors so that we can
  // initialize LazyStaticPtr objects with the "= { arg_value, ... }" syntax.
  // Clients of LazyStaticPtr must not access the data members directly.

  // Arguments for Type's c-tor
  // (unused NoArg-typed arguments consume either no space, or 1 byte to
  //  ensure address uniqueness):
  Arg1 arg1_;
  Arg2 arg2_;
  Arg3 arg3_;

  // The object we create and show.
  mutable Type *ptr_;

  // Ensures the ptr_ is initialized only once.
  mutable ::absl::Mutex mu_;

 private:
  template <typename A1, typename A2, typename A3>
  static Type *Factory(const A1 &a1, const A2 &a2, const A3 &a3) {
    return new Type(a1, a2, a3);
  }

  template <typename A1, typename A2>
  static Type *Factory(const A1 &a1, const A2 &a2, NoArg a3) {
    return new Type(a1, a2);
  }

  template <typename A1>
  static Type *Factory(const A1 &a1, NoArg a2, NoArg a3) {
    return new Type(a1);
  }

  static Type *Factory(NoArg a1, NoArg a2, NoArg a3) { return new Type(); }

  static void Initialize(const LazyStaticPtr *lsp) {
    lsp->ptr_ = Factory(lsp->arg1_, lsp->arg2_, lsp->arg3_);
  }
};

}  // namespace gtl

#endif  // STATECHART_PLATFORM_MAP_UTIL_H_
