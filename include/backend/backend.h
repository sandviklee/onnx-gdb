#pragma once
#include "ir/graph.h"
#include "ir/ports/backend_port.h"
#include <functional>
#include <onnx/onnx_pb.h>
#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>

namespace backend {

class OrtBackend : public ir::IBackendPort {
public:
  OrtBackend(ir::Graph &ir_graph,
             std::function<void(const std::string &, bool)> notify);

  void run_inference() override;
  void run_debug_inference() override;
  ir::TensorData
  run_mini_model(const std::string &bytes,
                 const std::vector<ir::TensorData> &inputs) override;

  void save_model(const onnx::ModelProto &model, const std::string &path);
  std::string get_name() override;

private:
  ir::Graph &ir_graph;
  std::function<void(const std::string &, bool)> notify;

  std::string onnx_model;
  std::vector<const char *> input_names;
  std::vector<const char *> output_names;

  Ort::Env env;
  Ort::MemoryInfo memory_info;
  Ort::Session session{nullptr};

  std::string serialize(const onnx::ModelProto &model);
  std::vector<Ort::Value> run_model(const Ort::RunOptions &options);
  void rebuild_session();
};

} // namespace backend
