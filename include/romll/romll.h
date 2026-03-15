#include "ui/graph.h"
#include <onnx/onnx_pb.h>
#include <onnxruntime/onnxruntime_cxx_api.h>

#pragma once
#ifndef ROMLL_H
#define ROMLL_H

class ROMLL {
private:
  std::string onnx_model = "";
  std::vector<float> input_data;
  std::vector<int64_t> input_shape;
  std::vector<char *> input_names;
  std::vector<char *> output_names;
  Ort::Env env;
  Ort::MemoryInfo memory_info;
  Ort::Session session{nullptr};

  std::string serialize(const onnx::ModelProto &onnx_protobuf);
  onnx::ModelProto parse_ui_graph(const Graph &graph);

public:
  ROMLL(Graph &graph, const std::vector<float> input_data,
        const std::vector<int64_t> input_shape,
        const std::vector<char *> &input_names,
        const std::vector<char *> &output_names);
  void save_model(const onnx::ModelProto &model, const std::string &path);
  std::vector<Ort::Value> run_model(const Ort::RunOptions &options);
};

#endif
