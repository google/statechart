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

// A set of macros that make the code more readable when using the early return
// pattern on encountering an unexpected error.

#ifndef STATE_CHART_LOGGING_H_
#define STATE_CHART_LOGGING_H_

#include <iostream>
#include <glog/logging.h>

#include "statechart/platform/logging.h"

// TODO(srgandhe): Also log using DM_LOG once we can access DMLogSink here.
// See b/27800056.
#define RETURN_VALUE_IF_MSG(condition, value, msg) \
  do {                                             \
    if (ABSL_PREDICT_FALSE((condition))) {         \
      LOG(DFATAL) << msg;                          \
      return (value);                              \
    }                                              \
  } while (false)

#define RETURN_FALSE_IF_MSG(condition, msg) \
  RETURN_VALUE_IF_MSG(condition, false, msg)

#define RETURN_FALSE_IF(condition)                              \
  RETURN_FALSE_IF_MSG(condition, "Returning false; condition (" \
                                     << #condition << ") is true.")

#define RETURN_NULL_IF_MSG(condition, msg) \
  RETURN_VALUE_IF_MSG(condition, nullptr, msg)

#define RETURN_NULL_IF(condition)                                \
  RETURN_NULL_IF_MSG(condition, "Returning nullptr; condition (" \
                                    << #condition << ") is true.")

#define RETURN_IF_MSG(condition, msg)      \
  do {                                     \
    if (ABSL_PREDICT_FALSE((condition))) { \
      LOG(DFATAL) << msg;                  \
      return;                              \
    }                                      \
  } while (false)

#define RETURN_IF(condition)                                      \
  RETURN_IF_MSG(condition, "Returning; condition (" << #condition \
                                                    << ") is true.")

#endif  // STATE_CHART_LOGGING_H_
