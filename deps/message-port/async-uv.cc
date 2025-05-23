/*
 * Copyright (c) 2025-present Samsung Electronics Co., Ltd
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

#include "async-uv.h"
#include <nd-logger.h>
#include <uv.h>
#include <functional>
#include "debug-mem-trace.h"

std::queue<AsyncUV::Task> AsyncUV::queue_;
std::mutex AsyncUV::queue_mutex_;

AsyncUV::AsyncUV(uv_loop_t* loop, Task task) : uv_h_(nullptr), task_(task) {
  if (loop && task) {
    Init(loop, task);
  }
  TRACE_ADD(ASYNC, this);
}

AsyncUV::~AsyncUV() {
  if (!uv_h_) {
    return;
  }

  TRACE(ASYNC, "~AsyncUV");
  TRACE_REMOVE(ASYNC, this);

  uv_close(reinterpret_cast<uv_handle_t*>(uv_h_), [](uv_handle_t* handle) {
    TRACE(ASYNC, "~uv_close");
    TRACE_REMOVE(ASYNC_UV, handle);
    delete reinterpret_cast<uv_async_t*>(handle);
  });
}

bool AsyncUV::Send(uv_loop_t* loop, Task task) {
  if (loop == nullptr) {
    return false;
  }
  return (new AsyncUV(loop, task))->Send();
}

size_t AsyncUV::EnqueueTask(Task task) {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  queue_.push(task);
  return queue_.size();
}

bool AsyncUV::DrainPendingTasks(uv_loop_t* loop) {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  TRACE(MSGPORT, "drain pending tasks", queue_.size());

  if (loop == nullptr) {
    return false;
  }

  while (!queue_.empty()) {
    AsyncUV::Send(loop, queue_.front());
    queue_.pop();
  }
  return true;
}

void AsyncUV::DeletePendingTasks() {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  TRACE(MSGPORT, "delete pending tasks", queue_.size());
  if (!queue_.empty()) {
    std::queue<Task> empty;
    std::swap(queue_, empty);
  }
}

bool AsyncUV::IsPendingTasksEmpty() {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  return queue_.empty();
}

#if 0
#define PLOG(fmt, ...) fprintf(stdout, "<lwnode-debug>" fmt "\n", ##__VA_ARGS__);
#else
#include <dlog.h>
#define PLOG(fmt, ...) dlog_print(DLOG_INFO, "LWNODE", "<lwnode-debug>" fmt, ##__VA_ARGS__);
#endif

void AsyncUV::Init(uv_loop_t* loop, Task task) {
  task_ = task;

  uv_h_ = new uv_async_t();
  uv_h_->data = this;
  uv_async_init(loop, uv_h_, [](uv_async_t* handle) {
    auto event = static_cast<AsyncUV*>(handle->data);
    if (event->task_) {
      auto now = std::chrono::system_clock::now();
      auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
                             now - event->start_time_)
                             .count();
      PLOG("(%.3f)[%.0lfms] (MessagePort)js callback: %p",
             uv_hrtime() / 1000000.0,
             duration_us / 1000.0,
             handle);
      event->task_(handle);
      duration_us = std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::system_clock::now() - now)
                        .count();
      PLOG("(%.3f)[%.0lfms] (MessagePort)done js callback: %p",
             uv_hrtime() / 1000000.0,
             duration_us / 1000.0,
             handle);
    }
    delete event;
  });
  TRACE_ADD(ASYNC_UV, uv_h_);
}

bool AsyncUV::Send() {
  if (!uv_h_) {
    return false;
  }
  PLOG("(%.3f)(MessagePort)send: %p", uv_hrtime() / 1000000.0, uv_h_);
  start_time_ = std::chrono::system_clock::now();
  uv_async_send(uv_h_);
  return true;
}
