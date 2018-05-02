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

#ifndef STATECHART_PLATFORM_TEST_UTIL_H_
#define STATECHART_PLATFORM_TEST_UTIL_H_

#include <string>
#include <type_traits>

#include "statechart/platform/protobuf.h"

#include <glog/logging.h>
#include <glog/log_severity.h>
#include <gmock/gmock.h>

//namespace proto2 {
//namespace contrib {
//namespace parse_proto {
//
//template <typename T>
//T ParseTextOrDie(const string& input) {
//  T result;
//  CHECK(proto2::TextFormat::ParseFromString(input, &result));
//  return result;
//}
//
//}  // namespace parse_proto
//}  // namespace contrib
//}  // namespace proto2

namespace base_logging {
using ::google::INFO;
using ::google::WARNING;
using ::google::ERROR;
using ::google::FATAL;
using ::google::NUM_SEVERITIES;
}  // namespace base_logging

namespace testing {

namespace proto {
template<typename T>
T IgnoringRepeatedFieldOrdering(T t) {
  return t;
}
} // namespace proto

MATCHER_P(EqualsProto, expected_str, "") {
  proto2::util::MessageDifferencer differencer;

  string diff;
  differencer.ReportDifferencesToString(&diff);

  // We must put 'expected' as the first argument here, as Compare()
  // reports the diff in terms of how the protobuf changes from the
  // first argument to the second argument.
  const auto expected =
      ::proto2::contrib::parse_proto::ParseTextOrDie<typename std::remove_cv<
          typename std::remove_reference<arg_type>::type>::type>(expected_str);
  bool same = differencer.Compare(expected, arg);
  if (!same) {
    *result_listener << diff;
  }
  return same;
}

MATCHER_P(EqualsProtobuf, expected, "") {
  proto2::util::MessageDifferencer differencer;

  string diff;
  differencer.ReportDifferencesToString(&diff);
  // We must put 'expected' as the first argument here, as Compare()
  // reports the diff in terms of how the protobuf changes from the
  // first argument to the second argument.
  bool same = differencer.Compare(expected, arg);
  if (!same) {
    *result_listener << diff;
  }
  return same;
}

enum LogCapturingState_ { kDoNotCaptureLogsYet };

class ScopedMockLog : public ::google::LogSink {
 public:
  // When a ScopedMockLog object is constructed, it starts to
  // intercept logs.
  ScopedMockLog() { ::google::AddLogSink(this); }

  explicit ScopedMockLog(LogCapturingState_ /* dummy */) {}

  // When the object is destructed, it stops intercepting logs.
  virtual ~ScopedMockLog() { ::google::RemoveLogSink(this); }

  void StartCapturingLogs() {
    ::google::AddLogSink(this);
  }

  // Implements the mock method:
  //
  //   void Log(LogSeverity severity, const string& file_path,
  //            const string& message);
  //
  // The second argument to Send() is the full path of the source file
  // in which the LOG() was issued.
  //
  // Note, that in a multi-threaded environment, all LOG() messages from a
  // single thread will be handled in sequence, but that cannot be guaranteed
  // for messages from different threads. In fact, if the same or multiple
  // expectations are matched on two threads concurrently, their actions will
  // be executed concurrently as well and may interleave.
  MOCK_METHOD3(Log, void(::google::LogSeverity severity,
                         const std::string& file_path,
                         const std::string& message));

 private:
  // Implements the send() virtual function in class LogSink.
  // Whenever a LOG() statement is executed, this function will be
  // invoked with information presented in the LOG().
  //
  // The method argument list is long and carries much information a
  // test usually doesn't care about, so we trim the list before
  // forwarding the call to Log(), which is much easier to use in
  // tests.
  //
  // We still cannot call Log() directly, as it may invoke other LOG()
  // messages, either due to Invoke, or due to an error logged in
  // Google C++ Mocking Framework code, which would trigger a deadlock
  // since a lock is held during send().
  //
  // Hence, we save the message for WaitTillSent() which will be called after
  // the lock on send() is released, and we'll call Log() inside
  // WaitTillSent(). Since while a single send() call may be running at a
  // time, multiple WaitTillSent() calls (along with the one send() call) may
  // be running simultaneously, we ensure thread-safety of the exchange between
  // send() and WaitTillSent(), and that for each message, LOG(), send(),
  // WaitTillSent() and Log() are executed in the same thread.
  virtual void send(::google::LogSeverity severity,
                    const char* full_filename,
                    const char* /*base_filename*/, int /*line*/,
                    const tm* /*tm_time*/,
                    const char* message, size_t message_len) {
    // We are only interested in the log severity, full file name, and
    // log message.
    message_info_.severity = severity;
    message_info_.file_path = full_filename;
    message_info_.message = std::string(message, message_len);
  }

  // Implements the WaitTillSent() virtual function in class LogSink.
  // It will be executed after send() and after the global logging lock is
  // released, so calls within it (or rather within the Log() method called
  // within) may also issue LOG() statements.
  //
  // LOG(), send(), WaitTillSent() and Log() will occur in the same thread for
  // a given log message.
  virtual void WaitTillSent() {
    // First, and very importantly, we save a copy of the message being
    // processed before calling Log(), since Log() may indirectly call send()
    // and WaitTillSent() in the same thread again.
    MessageInfo message_info = message_info_;
    Log(message_info.severity, message_info.file_path, message_info.message);
  }

  // All relevant information about a logged message that needs to be passed
  // from send() to WaitTillSent().
  struct MessageInfo {
    ::google::LogSeverity severity;
    std::string file_path;
    std::string message;
  };
  MessageInfo message_info_;
};

}  // namespace testing

#endif  // SRC_PLATFORM_PROTOBUF_MATCHER_H_
