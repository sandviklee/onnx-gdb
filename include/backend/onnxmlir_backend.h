#pragma once
#include "ir/graph.h"
#include "ir/ports/backend_port.h"
#include <functional>
#include <onnx/onnx_pb.h>
#include <string>

namespace backend {

// Compiles IR graphs to native code via the onnx-mlir CLI, then executes the
// compiled shared library using the OMTensorList runtime ABI
class OnnxMlirBackend : public ir::IBackendPort {
public:
  OnnxMlirBackend(ir::Graph &ir_graph,
                  std::function<void(const std::string &, bool)> notify);
  ~OnnxMlirBackend() override;

  void run_inference() override;
  void run_debug_inference() override;
  std::string get_name() override;
  ir::TensorData
  run_mini_model(const std::string &bytes,
                 const std::vector<ir::TensorData> &inputs) override;

private:
  ir::Graph &ir_graph;
  std::function<void(const std::string &, bool)> notify;

  std::string compiled_lib_path;
  void *model_lib = nullptr;

  using RunMainGraphFn = void *(*)(void *);
  RunMainGraphFn run_main_graph_fn = nullptr;

  bool compile_onnx(const std::string &onnx_path, std::string &error);
  bool load_model_lib(const std::string &lib_path, std::string &error);
  void unload_model();
  bool rebuild_from_ir(std::string &error);
  bool execute(std::string &error);
};

} // namespace backend
