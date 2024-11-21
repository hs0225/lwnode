/*
 * Copyright (c) 2021-present Samsung Electronics Co., Ltd
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

#include "lwnode-public.h"
#include "lwnode.h"
#include "node.h"

namespace LWNode {

int Node::Start(int argc, char** argv) {
  if (AULEventReceiver::getInstance()->start(argc, argv)) {
    LWNode::SystemInfo::getInstance()->add("aul");

    char* args[] = {"", "index.js", nullptr};
    return node::Start(2, args);
  }

  // started by command line
  return node::Start(argc, argv);
}

}  // namespace LWNode
