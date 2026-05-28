#include "backend/backend.h"
#include <fstream>

namespace backend {

OrtBackend::OrtBackend(ir::Graph &ir_graph,
                       std::function<void(const std::string &, bool)> notify)
    : ir_graph(ir_graph), notify(std::move(notify)),
      onnx_model(ir_graph.current_onnx_bytes()), env(),
      memory_info(
          Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
      session(Ort::Session(env, onnx_model.data(), onnx_model.size(),
                           Ort::SessionOptions{nullptr})) {
  for (auto *root : ir_graph.roots) {
    if (root->is_initializer)
      continue;
    input_names.push_back(root->label.c_str());
  }
  for (auto *leaf : ir_graph.leafs)
    output_names.push_back(leaf->label.c_str());
  ir_graph.topology_dirty = false;
}

std::string OrtBackend::serialize(const onnx::ModelProto &model) {
  std::string serialized;
  model.SerializeToString(&serialized);
  return serialized;
}

void OrtBackend::save_model(const onnx::ModelProto &model,
                            const std::string &path) {
  std::ofstream out(path, std::ios::binary);
  model.SerializeToOstream(&out);
}

void OrtBackend::rebuild_session() {
  onnx_model = ir_graph.current_onnx_bytes();
  session = Ort::Session(env, onnx_model.data(), onnx_model.size(),
                         Ort::SessionOptions{nullptr});
  input_names.clear();
  output_names.clear();
  for (auto *root : ir_graph.roots) {
    if (root->is_initializer)
      continue;
    input_names.push_back(root->label.c_str());
  }
  for (auto *leaf : ir_graph.leafs)
    output_names.push_back(leaf->label.c_str());
}

std::vector<Ort::Value> OrtBackend::run_model(const Ort::RunOptions &options) {
  std::vector<Ort::Value> input_tensors;

  for (auto *root : ir_graph.roots) {
    if (root->is_initializer)
      continue;
    auto &data = root->values;
    std::vector<int64_t> shape;
    if (!root->shape_dims.empty()) {
      for (int d : root->shape_dims)
        shape.push_back((int64_t)d);
    } else {
      shape = {(int64_t)data.size()};
    }
    input_tensors.push_back(Ort::Value::CreateTensor(
        memory_info, data.data(), data.size(), shape.data(), shape.size()));
  }

  return session.Run(options, input_names.data(), input_tensors.data(),
                     input_names.size(), output_names.data(),
                     output_names.size());
}

void OrtBackend::run_inference() {
  if (ir_graph.topology_dirty)
    rebuild_session();

  Ort::RunOptions run_options{nullptr};
  try {
    std::vector<Ort::Value> outputs = run_model(run_options);

    for (size_t i = 0; i < outputs.size(); i++) {
      auto info = outputs[i].GetTensorTypeAndShapeInfo();
      auto shape = info.GetShape();
      float *data = outputs[i].GetTensorMutableData<float>();
      int64_t size = info.GetElementCount();

      ir_graph.leafs[i]->values.resize(size);
      for (int64_t j = 0; j < size; j++)
        ir_graph.leafs[i]->values[j] = data[j];
      ir_graph.leafs[i]->has_results = true;
    }
  } catch (const Ort::Exception &e) {
    notify(std::string("ONNX Runtime error: ") + e.what(), true);
  }
}

void OrtBackend::run_debug_inference() {
  try {
    ir_graph.debug_walk(*this);
    notify("Debug: wire values visible", false);
  } catch (const Ort::Exception &e) {
    notify(std::string("Debug error: ") + e.what(), true);
  } catch (const std::exception &e) {
    notify(std::string("Debug error: ") + e.what(), true);
  }
}

ir::TensorData
OrtBackend::run_mini_model(const std::string &bytes,
                           const std::vector<ir::TensorData> &inputs) {
  Ort::Session sess(env, bytes.data(), bytes.size(),
                    Ort::SessionOptions{nullptr});

  Ort::AllocatorWithDefaultOptions alloc;
  std::vector<Ort::AllocatedStringPtr> name_holders;
  std::vector<const char *> in_ptrs;
  std::vector<Ort::Value> in_vals;

  size_t n_in = sess.GetInputCount();
  if (inputs.size() != n_in)
    throw std::runtime_error("run_mini_model: input count mismatch");

  for (size_t i = 0; i < n_in; i++) {
    auto name = sess.GetInputNameAllocated(i, alloc);
    const ir::TensorData &td = inputs[i];
    in_vals.push_back(Ort::Value::CreateTensor(
        memory_info, const_cast<float *>(td.values.data()), td.values.size(),
        const_cast<int64_t *>(td.shape.data()), td.shape.size()));
    in_ptrs.push_back(name.get());
    name_holders.push_back(std::move(name));
  }

  auto out_name = sess.GetOutputNameAllocated(0, alloc);
  const char *out_ptr = out_name.get();

  Ort::RunOptions run_opts{nullptr};
  auto results = sess.Run(run_opts, in_ptrs.data(), in_vals.data(),
                          in_ptrs.size(), &out_ptr, 1);

  ir::TensorData out;
  auto info = results[0].GetTensorTypeAndShapeInfo();
  out.shape = info.GetShape();
  int64_t count = info.GetElementCount();
  const float *data = results[0].GetTensorData<float>();
  out.values.assign(data, data + count);
  return out;
}

std::string OrtBackend::get_name() { return "ort"; }

} // namespace backend
