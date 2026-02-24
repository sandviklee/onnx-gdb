#include <onnx/onnx_pb.h>
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <string>
#pragma once
#ifndef BOILPARSER_H
#define BOILPARSER_H

class Boil {
private:
  std::string onnx_model = "";
  std::vector<float> input_data;
  std::vector<char *> input_names;
  std::vector<char *> output_names;
  std::vector<int64_t> input_shape;
  Ort::Env env;
  Ort::MemoryInfo memory_info;
  Ort::Session session{nullptr};

  std::string serialize(const onnx::ModelProto &onnx_protobuf);

public:
  Boil(const onnx::ModelProto onnx_protobuf,
       const std::vector<float> input_data,
       const std::vector<int64_t> input_shape,
       const std::vector<char *> input_names,
       const std::vector<char *> output_names);
  void save_model(const onnx::ModelProto &model, const std::string &path);
  std::vector<Ort::Value> run_model(const Ort::RunOptions &options);
};

#endif
