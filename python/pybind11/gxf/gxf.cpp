/*
 * SPDX-FileCopyrightText: Copyright (c) 2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // for unordered_map

#include <cstdint>
#include <memory>
#include <string>

#include "../core/core.hpp"
#include "../core/dl_converter.hpp"
#include "gxf.hpp"

#include "holoscan/core/component_spec.hpp"
#include "holoscan/core/fragment.hpp"
#include "holoscan/core/gxf/gxf_component.hpp"
#include "holoscan/core/gxf/gxf_condition.hpp"
#include "holoscan/core/gxf/gxf_execution_context.hpp"
#include "holoscan/core/gxf/gxf_extension_registrar.hpp"
#include "holoscan/core/gxf/gxf_io_context.hpp"
#include "holoscan/core/gxf/gxf_operator.hpp"
#include "holoscan/core/gxf/gxf_resource.hpp"
#include "holoscan/core/gxf/gxf_tensor.hpp"
#include "holoscan/core/gxf/gxf_wrapper.hpp"

#include "gxf/core/gxf.h"

using std::string_literals::operator""s;
using pybind11::literals::operator""_a;

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

// note on gxf typedefs:
//     typedef void* gxf_context_t;
//     gxf_result_t is a C-style enum where GXF_SUCCESS is 0

namespace holoscan::ops {

// define trampoline class for GXFOperator to allow overriding virtual gxf_typename

class PyGXFOperator : public GXFOperator {
 public:
  /* Inherit the constructors */
  using GXFOperator::GXFOperator;

  /* Trampolines (need one for each virtual function) */
  const char* gxf_typename() const override {
    /* <Return type>, <Parent Class>, <Name of C++ function>, <Argument(s)> */
    PYBIND11_OVERRIDE_PURE(const char*, GXFOperator, gxf_typename);
  }
};

}  // namespace holoscan::ops

namespace holoscan {

static const gxf_tid_t default_tid = {0, 0};

PYBIND11_MODULE(_gxf, m) {
  m.doc() = R"pbdoc(
        Holoscan SDK Python Bindings
        ---------------------------------------
        .. currentmodule:: _gxf
        .. autosummary::
           :toctree: _generate
           add
           subtract
    )pbdoc";

#ifdef VERSION_INFO
  m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
  m.attr("__version__") = "dev";
#endif

  // Note: here we copy load_extensions from GXF's Python bindings for convenience
  m.def(
      "load_extensions",
      [](uint64_t context,
         const std::vector<std::string>& extension_filenames,
         const std::vector<std::string>& manifest_filenames,
         const std::string& base_directory) {
        std::vector<const char*> extension_filenames_cstr(extension_filenames.size());
        for (size_t i = 0; i < extension_filenames.size(); i++) {
          extension_filenames_cstr[i] = extension_filenames[i].c_str();
        }

        std::vector<const char*> manifest_filenames_cstr(manifest_filenames.size());
        for (size_t i = 0; i < manifest_filenames.size(); i++) {
          manifest_filenames_cstr[i] = manifest_filenames[i].c_str();
        }

        const GxfLoadExtensionsInfo info{
            extension_filenames_cstr.empty() ? nullptr : extension_filenames_cstr.data(),
            static_cast<uint32_t>(extension_filenames_cstr.size()),
            manifest_filenames_cstr.empty() ? nullptr : manifest_filenames_cstr.data(),
            static_cast<uint32_t>(manifest_filenames_cstr.size()),
            base_directory.c_str()};

        const gxf_result_t result =
            GxfLoadExtensions(reinterpret_cast<gxf_context_t>(context), &info);
        if (result != GXF_SUCCESS) { throw pybind11::value_error(GxfResultStr(result)); }
      },
      "Loads GXF extension libraries",
      "context"_a,
      "extension_filenames"_a = std::vector<std::string>{},
      "manifest_filenames"_a = std::vector<std::string>{},
      "base_directory"_a = "");

  py::class_<gxf::Entity, std::shared_ptr<gxf::Entity>>(m, "Entity", doc::Entity::doc_Entity)
      .def(py::init<>(), doc::Entity::doc_Entity);

  py::class_<PyEntity, gxf::Entity, std::shared_ptr<PyEntity>>(
      m, "PyEntity", doc::Entity::doc_Entity)
      .def(py::init(&PyEntity::py_create), doc::Entity::doc_Entity)
      .def("get", &PyEntity::py_get, "name"_a = "", doc::Entity::doc_get)
      .def("add", &PyEntity::py_add, "obj"_a, "name"_a = "");

  py::class_<gxf::GXFTensor, std::shared_ptr<gxf::GXFTensor>>(
      m, "GXFTensor", doc::GXFTensor::doc_GXFTensor)
      .def(py::init<>(), doc::GXFTensor::doc_GXFTensor);

  py::class_<gxf::GXFComponent, std::shared_ptr<gxf::GXFComponent>>(
      m, "GXFComponent", doc::GXFComponent::doc_GXFComponent)
      .def(py::init<>(), doc::GXFComponent::doc_GXFComponent)
      .def_property("gxf_context",
                    py::overload_cast<>(&gxf::GXFComponent::gxf_context, py::const_),
                    py::overload_cast<void*>(&gxf::GXFComponent::gxf_context),
                    doc::GXFComponent::doc_gxf_context)
      .def_property("gxf_eid",
                    py::overload_cast<>(&gxf::GXFComponent::gxf_eid, py::const_),
                    py::overload_cast<gxf_uid_t>(&gxf::GXFComponent::gxf_eid),
                    doc::GXFComponent::doc_gxf_eid)
      .def_property("gxf_cid",
                    py::overload_cast<>(&gxf::GXFComponent::gxf_cid, py::const_),
                    py::overload_cast<gxf_uid_t>(&gxf::GXFComponent::gxf_cid),
                    doc::GXFComponent::doc_gxf_cid)
      .def_property("gxf_cname",
                    py::overload_cast<>(&gxf::GXFComponent::gxf_cname),
                    py::overload_cast<const std::string&>(&gxf::GXFComponent::gxf_cname),
                    doc::GXFComponent::doc_gxf_cname)
      .def("gxf_initialize",
           &gxf::GXFComponent::gxf_initialize,
           doc::GXFComponent::doc_gxf_initialize);

  py::class_<gxf::GXFResource, Resource, gxf::GXFComponent, std::shared_ptr<gxf::GXFResource>>(
      m, "GXFResource", doc::GXFResource::doc_GXFResource)
      .def(py::init<>(), doc::GXFResource::doc_GXFResource)
      .def("initialize", &gxf::GXFResource::initialize, doc::GXFResource::doc_initialize);

  py::class_<gxf::GXFCondition, Condition, gxf::GXFComponent, std::shared_ptr<gxf::GXFCondition>>(
      m, "GXFCondition", doc::GXFCondition::doc_GXFCondition)
      .def(py::init<>(), doc::GXFCondition::doc_GXFCondition)
      .def("initialize", &gxf::GXFCondition::initialize, doc::GXFCondition::doc_initialize);

  py::class_<ops::GXFOperator, ops::PyGXFOperator, Operator, std::shared_ptr<ops::GXFOperator>>(
      m, "GXFOperator", doc::GXFOperator::doc_GXFOperator)
      .def(py::init<>(), doc::GXFOperator::doc_GXFOperator)
      // .def("initialize", &ops::GXFOperator::initialize)
      .def_property_readonly(
          "gxf_context", &ops::GXFOperator::gxf_context, doc::GXFOperator::doc_gxf_context)
      .def_property("gxf_eid",
                    py::overload_cast<>(&ops::GXFOperator::gxf_eid, py::const_),
                    py::overload_cast<gxf_uid_t>(&ops::GXFOperator::gxf_eid),
                    doc::GXFOperator::doc_gxf_eid)
      .def_property("gxf_cid",
                    py::overload_cast<>(&ops::GXFOperator::gxf_cid, py::const_),
                    py::overload_cast<gxf_uid_t>(&ops::GXFOperator::gxf_cid),
                    doc::GXFOperator::doc_gxf_cid);
  // Note: have not wrapped register_converter as it is currently not needed from Python.

  py::class_<gxf::GXFInputContext, InputContext, std::shared_ptr<gxf::GXFInputContext>>(
      m, "GXFInputContext", R"doc(GXF input context.)doc")
      .def(py::init<gxf_context_t, ops::GXFOperator*>(),
           "context"_a,
           "op"_a,
           doc::GXFInputContext::doc_GXFInputContext)
      .def(
          "receive", [](const InputContext&, const std::string&) { return py::none(); }, "name"_a);

  py::class_<gxf::GXFOutputContext, OutputContext, std::shared_ptr<gxf::GXFOutputContext>>(
      m, "GXFOutputContext", R"doc(GXF output context.)doc")
      .def(py::init<gxf_context_t, ops::GXFOperator*>(),
           "context"_a,
           "op"_a,
           doc::GXFOutputContext::doc_GXFOutputContext)
      .def(
          "emit",
          [](const OutputContext&, py::object&, const std::string&) {},
          "data"_a,
          "name"_a = "");

  py::class_<gxf::GXFExecutionContext, ExecutionContext, std::shared_ptr<gxf::GXFExecutionContext>>(
      m, "GXFExecutionContext", R"doc(GXF execution context.)doc")
      .def(py::init<gxf_context_t, ops::GXFOperator*>(),
           "context"_a,
           "op"_a,
           doc::GXFExecutionContext::doc_GXFExecutionContext);

  py::class_<gxf::GXFExtensionRegistrar> gxf_extension_registrar(
      m, "GXFExtensionRegistrar", doc::GXFExtensionRegistrar::doc_GXFExtensionRegistrar);

  py::class_<gxf_tid_t>(m, "gxf_tid_t");

  gxf_extension_registrar
      .def(py::init<gxf_context_t, const char*, const char*, gxf_tid_t>(),
           "context"_a,
           "extension_name"_a,
           "extension_description"_a = "",
           "tid"_a = default_tid,
           doc::GXFExtensionRegistrar::doc_GXFExtensionRegistrar)
      .def("create_random_tid",
           &gxf::GXFExtensionRegistrar::create_random_tid,
           doc::GXFExtensionRegistrar::doc_create_random_tid)
      .def("is_allocated",
           &gxf::GXFExtensionRegistrar::is_allocated,
           "tid"_a,
           "kind"_a,
           doc::GXFExtensionRegistrar::doc_is_allocated)
      .def("allocate_tid",
           &gxf::GXFExtensionRegistrar::allocate_tid,
           "kind"_a,
           doc::GXFExtensionRegistrar::doc_allocate_tid)
      .def("register_extension",
           &gxf::GXFExtensionRegistrar::register_extension,
           doc::GXFExtensionRegistrar::doc_register_extension)
      .def("reset",
           &gxf::GXFExtensionRegistrar::reset,
           "context"_a,
           "extension_name"_a,
           "extension_description"_a = "",
           "tid"_a = default_tid,
           doc::GXFExtensionRegistrar::doc_reset)
      // templated add_component methods
      .def("add_component",
           py::overload_cast<const char*, gxf_tid_t>(
               &gxf::GXFExtensionRegistrar::add_component<gxf::GXFWrapper, ::nvidia::gxf::Codelet>),
           "description"_a = "",
           "tid"_a = default_tid,
           doc::GXFExtensionRegistrar::doc_add_component)
      // templated add_type methods
      .def(
          "add_type",
          py::overload_cast<const char*, gxf_tid_t>(&gxf::GXFExtensionRegistrar::add_type<Message>),
          "description"_a = "",
          "tid"_a = default_tid,
          doc::GXFExtensionRegistrar::doc_add_type);

  py::enum_<gxf::GXFExtensionRegistrar::TypeKind>(gxf_extension_registrar, "TypeKind")
      .value("Extension", gxf::GXFExtensionRegistrar::TypeKind::kExtension)
      .value("Component", gxf::GXFExtensionRegistrar::TypeKind::kComponent);

  py::class_<gxf::GXFWrapper>(m, "GXFWrapper", doc::GXFWrapper::doc_GXFWrapper)
      .def(py::init<>(), doc::GXFWrapper::doc_GXFWrapper)
      .def("initialize", &gxf::GXFWrapper::initialize, doc::GXFWrapper::doc_initialize)
      .def("deinitialize", &gxf::GXFWrapper::deinitialize, doc::GXFWrapper::doc_deinitialize)
      .def("registerInterface",
           &gxf::GXFWrapper::registerInterface,
           doc::GXFWrapper::doc_registerInterface)
      .def("start", &gxf::GXFWrapper::start, doc::GXFWrapper::doc_start)
      .def("tick", &gxf::GXFWrapper::tick, doc::GXFWrapper::doc_tick)
      .def("stop", &gxf::GXFWrapper::stop, doc::GXFWrapper::doc_stop)
      .def("set_operator", &gxf::GXFWrapper::set_operator, doc::GXFWrapper::doc_set_operator);
}  // PYBIND11_MODULE

////////////////////////////////////////////////////////////////////////////////////////////////////
// PyEntity definition
////////////////////////////////////////////////////////////////////////////////////////////////////

PyEntity PyEntity::py_create(const PyExecutionContext& ctx) {
  auto result = nvidia::gxf::Entity::New(ctx.context());
  if (!result) { throw std::runtime_error("Failed to create entity"); }
  return static_cast<PyEntity>(result.value());
}

py::object PyEntity::py_get(const char* name) const {
  auto tensor = get<Tensor>(name);
  if (!tensor) { return py::none(); }

  auto py_tensor = py::cast(tensor);

  // Set array interface attributes
  set_array_interface(py_tensor, tensor->dl_ctx());
  // Set dlpack attributes
  set_dlpack_interface(py_tensor, tensor->dl_ctx());
  return py_tensor;
}

py::object PyEntity::py_add(const py::object& value, const char* name) {
  if (py::isinstance<PyTensor>(value)) {
    std::shared_ptr<Tensor> tensor =
        std::static_pointer_cast<Tensor>(py::cast<std::shared_ptr<PyTensor>>(value));
    add<Tensor>(tensor, name);
    return value;
  }
  return py::none();
}

}  // namespace holoscan
