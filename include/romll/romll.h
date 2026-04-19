#include "ui/graph.h"
#include <onnx/onnx_pb.h>
#include <onnxruntime_cxx_api.h>

#pragma once
#ifndef ROMLL_H
#define ROMLL_H

class ROMLL {
private:
  std::string onnx_model = "";
  std::vector<float> input_data;
  std::vector<int64_t> input_shape;
  std::vector<const char *> input_names;
  std::vector<const char *> output_names;
  Ort::Env env;
  Ort::MemoryInfo memory_info;
  Ort::Session session{nullptr};

  std::string serialize(const onnx::ModelProto &onnx_protobuf);
  onnx::ModelProto parse_ui_graph(const Graph &graph);
  std::vector<Ort::Value> run_model(const Ort::RunOptions &options);
  void rebuild_session();

public:
  ROMLL(Graph &graph);

  Graph &graph;

  void save_model(const onnx::ModelProto &model, const std::string &path);
  void run_inference();
};

#endif
