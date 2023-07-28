/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "holoscan/core/conditions/gxf/asynchronous.hpp"

#include <string>

#include "holoscan/core/component_spec.hpp"

namespace holoscan {

AsynchronousCondition::AsynchronousCondition(const std::string& name,
                                             nvidia::gxf::AsynchronousSchedulingTerm* term)
    : GXFCondition(name, term) {
  if (term) {
    // no parameters to configure for this condition type
  } else {
    HOLOSCAN_LOG_ERROR("AsynchronousCondition: term is null");
  }
}

void AsynchronousCondition::setup(ComponentSpec& spec) {
  (void)spec;  // no parameters to set
}

void AsynchronousCondition::event_state(AsynchronousEventState state) {
  if (gxf_cptr_) {
    nvidia::gxf::AsynchronousSchedulingTerm* asynchronous_scheduling_term =
        static_cast<nvidia::gxf::AsynchronousSchedulingTerm*>(gxf_cptr_);
    asynchronous_scheduling_term->setEventState(state);
  }
  event_state_ = state;
}

AsynchronousEventState AsynchronousCondition::event_state() const {
  if (gxf_cptr_) {
    nvidia::gxf::AsynchronousSchedulingTerm* asynchronous_scheduling_term =
        static_cast<nvidia::gxf::AsynchronousSchedulingTerm*>(gxf_cptr_);
    return asynchronous_scheduling_term->getEventState();
  }
  return event_state_;
}

}  // namespace holoscan