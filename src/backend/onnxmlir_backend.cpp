#include "backend/onnxmlir_backend.h"
#include <chrono>
#include <cstdio>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <onnx-mlir/Runtime/OMTensor.h>
#include <onnx-mlir/Runtime/OMTensorList.h>
#include <onnx-mlir/Runtime/OnnxDataType.h>

#ifndef ROMLL_ONNXMLIR_BIN
#define ROMLL_ONNXMLIR_BIN "onnx-mlir"
#endif

namespace backend {

static constexpr const char *COMPILED_LIB_EXT = ".so";

OnnxMlirBackend::OnnxMlirBackend(
    ir::Graph &ir_graph, std::function<void(const std::string &, bool)> notify)
    : ir_graph(ir_graph), notify(std::move(notify)) {}

OnnxMlirBackend::~OnnxMlirBackend() { unload_model(); }

template <typename T>
static bool load_sym(void *handle, const char *name, T &out, std::string &err) {
  void *sym = dlsym(handle, name);
  if (!sym) {
    err = std::string("missing symbol ") + name + ": " + dlerror();
    return false;
  }
  out = reinterpret_cast<T>(sym);
  return true;
}

bool OnnxMlirBackend::compile_onnx(const std::string &onnx_path,
                                   std::string &error) {
  std::string stem = onnx_path.substr(0, onnx_path.rfind('.'));
  std::string cmd = std::string(ROMLL_ONNXMLIR_BIN) + " --EmitLib \"" +
                    onnx_path + "\" -o \"" + stem + "\" 2>&1";

  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    error = "failed to invoke onnx-mlir (is it in PATH?)";
    return false;
  }
  std::string output;
  char buf[256];
  while (fgets(buf, sizeof(buf), pipe))
    output += buf;
  int status = pclose(pipe);
  if (status != 0) {
    error = output.empty() ? "onnx-mlir exited with error" : output;
    return false;
  }

  compiled_lib_path = stem + COMPILED_LIB_EXT;
  return true;
}

bool OnnxMlirBackend::load_model_lib(const std::string &lib_path,
                                     std::string &error) {
  unload_model();
  model_lib = dlopen(lib_path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!model_lib) {
    error = std::string(dlerror());
    return false;
  }
  return load_sym(model_lib, "run_main_graph", run_main_graph_fn, error);
}

void OnnxMlirBackend::unload_model() {
  run_main_graph_fn = nullptr;
  if (model_lib) {
    dlclose(model_lib);
    model_lib = nullptr;
  }
}

bool OnnxMlirBackend::rebuild_from_ir(std::string &error) {
  auto tmp = std::filesystem::temp_directory_path();
  auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
  std::string onnx_path =
      (tmp / ("onnx-dbg" + std::to_string(ts) + ".onnx")).string();

  const std::string &bytes = ir_graph.current_onnx_bytes();
  {
    std::ofstream out(onnx_path, std::ios::binary);
    out.write(bytes.data(), (std::streamsize)bytes.size());
    if (!out) {
      error = "failed to write temp ONNX model to " + onnx_path;
      return false;
    }
  }

  std::string compile_err;
  if (!compile_onnx(onnx_path, compile_err)) {
    std::filesystem::remove(onnx_path);
    error = "onnx-mlir compilation failed: " + compile_err;
    return false;
  }

  std::filesystem::remove(onnx_path);

  std::string load_err;
  if (!load_model_lib(compiled_lib_path, load_err)) {
    error = "failed to load compiled model: " + load_err;
    return false;
  }

  ir_graph.topology_dirty = false;
  return true;
}

bool OnnxMlirBackend::execute(std::string &error) {
  if (!run_main_graph_fn) {
    error = "model not compiled";
    return false;
  }

  std::vector<std::vector<int64_t>> shapes;
  std::vector<OMTensor *> input_tensors;
  std::vector<ir::Node *> active_roots;
  for (auto *root : ir_graph.roots) {
    if (root->is_initializer)
      continue;
    active_roots.push_back(root);
    std::vector<int64_t> shape;
    for (int d : root->shape_dims)
      shape.push_back((int64_t)d);
    if (shape.empty())
      shape = {(int64_t)root->values.size()};
    shapes.push_back(std::move(shape));
  }

  for (size_t i = 0; i < active_roots.size(); i++) {
    auto *root = active_roots[i];
    OMTensor *t = omTensorCreate(root->values.data(), shapes[i].data(),
                                 (int64_t)shapes[i].size(), ONNX_TYPE_FLOAT);
    input_tensors.push_back(t);
  }

  OMTensorList *in_list =
      omTensorListCreate(input_tensors.data(), (int64_t)input_tensors.size());
  OMTensorList *out_list =
      static_cast<OMTensorList *>(run_main_graph_fn(in_list));

  omTensorListDestroyShallow(in_list);
  for (OMTensor *t : input_tensors)
    omTensorDestroy(t);

  if (!out_list) {
    error = "run_main_graph returned null";
    return false;
  }

  for (size_t i = 0; i < ir_graph.leafs.size(); i++) {
    OMTensor *t = omTensorListGetOmtByIndex(out_list, (int64_t)i);
    if (!t)
      continue;
    int64_t count = omTensorGetNumElems(t);
    float *data = static_cast<float *>(omTensorGetDataPtr(t));
    ir_graph.leafs[i]->values.assign(data, data + count);
    ir_graph.leafs[i]->has_results = true;
  }

  omTensorListDestroy(out_list);
  return true;
}

void OnnxMlirBackend::run_inference() {
  if (ir_graph.topology_dirty) {
    std::string err;
    if (!rebuild_from_ir(err)) {
      notify("Compile error: " + err, true);
      return;
    }
  }
  if (!run_main_graph_fn)
    return;
  std::string err;
  if (!execute(err))
    notify("Inference error: " + err, true);
}

void OnnxMlirBackend::run_debug_inference() {
  try {
    ir_graph.debug_walk(*this);
    notify("Debug: wire values visible", false);
  } catch (const std::exception &e) {
    notify(std::string("Debug error: ") + e.what(), true);
  }
}

ir::TensorData
OnnxMlirBackend::run_mini_model(const std::string &bytes,
                                const std::vector<ir::TensorData> &inputs) {
  auto tmp = std::filesystem::temp_directory_path();
  auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
  std::string onnx_path =
      (tmp / ("ir-session-onnx-dbg_" + std::to_string(ts) + ".onnx")).string();

  {
    std::ofstream out(onnx_path, std::ios::binary);
    out.write(bytes.data(), (std::streamsize)bytes.size());
    if (!out)
      throw std::runtime_error("failed to write temp ONNX to " + onnx_path);
  }

  std::string compile_err;
  if (!compile_onnx(onnx_path, compile_err)) {
    std::filesystem::remove(onnx_path);
    throw std::runtime_error("onnx-mlir compilation failed: " + compile_err);
  }
  std::filesystem::remove(onnx_path);

  void *lib = dlopen(compiled_lib_path.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (!lib)
    throw std::runtime_error(std::string("dlopen failed: ") + dlerror());
  RunMainGraphFn run_fn = nullptr;
  std::string sym_err;
  if (!load_sym(lib, "run_main_graph", run_fn, sym_err)) {
    dlclose(lib);
    throw std::runtime_error(sym_err);
  }

  std::vector<std::vector<int64_t>> shapes;
  std::vector<OMTensor *> input_tensors;
  shapes.reserve(inputs.size());
  for (const auto &td : inputs) {
    std::vector<int64_t> shape = td.shape;
    if (shape.empty())
      shape = {(int64_t)td.values.size()};
    shapes.push_back(std::move(shape));
  }
  for (size_t i = 0; i < inputs.size(); i++) {
    OMTensor *t = omTensorCreate(const_cast<float *>(inputs[i].values.data()),
                                 shapes[i].data(), (int64_t)shapes[i].size(),
                                 ONNX_TYPE_FLOAT);
    input_tensors.push_back(t);
  }

  OMTensorList *in_list =
      omTensorListCreate(input_tensors.data(), (int64_t)input_tensors.size());
  OMTensorList *out_list = static_cast<OMTensorList *>(run_fn(in_list));

  omTensorListDestroyShallow(in_list);
  for (OMTensor *t : input_tensors)
    omTensorDestroy(t);

  if (!out_list) {
    dlclose(lib);
    throw std::runtime_error("run_main_graph returned null");
  }

  ir::TensorData out;
  OMTensor *t = omTensorListGetOmtByIndex(out_list, 0);
  if (t) {
    int64_t count = omTensorGetNumElems(t);
    float *data = static_cast<float *>(omTensorGetDataPtr(t));
    out.values.assign(data, data + count);
    int64_t rank = omTensorGetRank(t);
    const int64_t *shape_ptr = omTensorGetShape(t);
    if (shape_ptr && rank > 0)
      out.shape.assign(shape_ptr, shape_ptr + rank);
  }
  omTensorListDestroy(out_list);
  dlclose(lib);
  return out;
}

std::string OnnxMlirBackend::get_name() { return "onnx-mlir"; }

} // namespace backend
