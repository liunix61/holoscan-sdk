/*
 * SPDX-FileCopyrightText: Copyright (c) 2023-2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#ifndef PYBIND11_CORE_ARG_HPP
#define PYBIND11_CORE_ARG_HPP

#include <pybind11/pybind11.h>

#include <unordered_map>

#include "holoscan/core/arg.hpp"

namespace py = pybind11;

namespace holoscan {

void init_arg(py::module_&);

}  // namespace holoscan

#endif /* PYBIND11_CORE_ARG_HPP */
