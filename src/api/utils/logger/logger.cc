/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "logger.h"
#include <uv.h>
#include <cstdarg>
#include <cstdio>
#include <iomanip>  // for setfill and setw

// Dlog
#ifdef HOST_TIZEN
#include <dlog.h>
#endif

void DlogOut::flush(std::stringstream& ss,
                    std::shared_ptr<Output::Config> config) {
  auto c =
      config ? std::static_pointer_cast<DLogConfig>(config) : LogKind::lwnode();
#ifdef HOST_TIZEN
  dlog_print(DLOG_INFO, c->tag.c_str(), "%s", ss.str().c_str());
#else
  // For testing. StdOut will be used to flush buffers through stdout.
  std::cout << std::left << std::setfill(' ') << std::setw(6) << c->tag << " "
            << ss.str();
#endif
}

void DlogOut::appendEndOfLine(std::stringstream& ss) {
#ifdef HOST_TIZEN
  /* NOTHING */
#else
  // For testing. StdOut will be used to flush buffers through stdout.
  ss << std::endl;
#endif
};

// LogKind
LogKind* LogKind::getInstance() {
  static LogKind kind;
  return &kind;
}

LogKind::LogKind() {
  user_ = std::make_shared<DLogConfig>("USER");
  lwnode_ = std::make_shared<DLogConfig>("LWNODE");
}

PerformanceLog::PerformanceLog(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  message_ = buffer;
  start_time_ = std::chrono::high_resolution_clock::now();

  PLOG("(%.3f)%s", uv_hrtime() / 1000000.0, message_.c_str());
}

PerformanceLog::~PerformanceLog() {
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
      end_time - start_time_);
  double duration_ms = duration_us.count() / 1000.0;

  PLOG("(%.3f)[%.0lfms] %s",
         uv_hrtime() / 1000000.0,
         duration_ms,
         message_.c_str());
}
