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

// IWYU pragma: private, include "src/internal/model/model.h"
// IWYU pragma: friend src/internal/model/*

#ifndef STATE_CHART_INTERNAL_MODEL_MODEL_ELEMENT_H_
#define STATE_CHART_INTERNAL_MODEL_MODEL_ELEMENT_H_

namespace state_chart {
namespace model {

// A root superclass used for memory management of all instances of model
// related classes except the model itself.
// It only contains a single virtual destructor.
class ModelElement {
 public:
  virtual ~ModelElement();
};

}  // namespace model
}  // namespace state_chart

#endif  // STATE_CHART_INTERNAL_MODEL_MODEL_ELEMENT_H_
