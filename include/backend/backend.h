#pragma once
#include "ir/graph.h"
#include "ir/ports/backend_port.h"
#include <functional>
#include <onnx/onnx_pb.h>
#include <onnxruntime_cxx_api.h>
#include <string>
#include <vector>

namespace backend {

class ROMLL : public ir::IBackendPort {
public:
    ROMLL(ir::Graph& ir_graph,
          std::function<void(const std::string&, bool)> notify);

    void run_inference() override;
    void run_debug_inference() override;
    bool load_onnx_file(const std::string& path, std::string& error) override;

    void save_model(const onnx::ModelProto& model, const std::string& path);

private:
    ir::Graph& ir_graph;
    std::function<void(const std::string&, bool)> notify;

    std::string onnx_model;
    std::vector<const char*> input_names;
    std::vector<const char*> output_names;

    Ort::Env env;
    Ort::MemoryInfo memory_info;
    Ort::Session session{nullptr};

    std::string serialize(const onnx::ModelProto& model);
    onnx::ModelProto parse_ir_graph();
    std::vector<Ort::Value> run_model(const Ort::RunOptions& options);
    void rebuild_session();
    void build_graph_from_onnx(const onnx::ModelProto& model, std::string& warnings);
};

} // namespace backend
