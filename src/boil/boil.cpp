#include <boil/boil.h>
#include <fstream>
#include <iostream>

Boil::Boil(const onnx::ModelProto onnx_protobuf,
           const std::vector<float> input_data,
           const std::vector<int64_t> input_shape,
           const std::vector<char *> input_names,
           const std::vector<char *> output_names)
    : onnx_model(serialize(onnx_protobuf)), input_data(input_data),
      input_names(input_names), output_names(output_names),
      input_shape(input_shape), env(),
      memory_info(
          Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
      session(Ort::Session(env, onnx_model.data(), onnx_model.size(),
                           Ort::SessionOptions{nullptr})) {};

std::string Boil::serialize(const onnx::ModelProto &model) {
  std::string serialized_model;
  model.SerializeToString(&serialized_model);
  return serialized_model;
};

void Boil::save_model(const onnx::ModelProto &model, const std::string &path) {
  std::ofstream out(path, std::ios::binary);
  model.SerializeToOstream(&out);
};

std::vector<Ort::Value> Boil::run_model(const Ort::RunOptions &options) {
  Ort::Value input_tensor = Ort::Value::CreateTensor(
      memory_info, input_data.data(), input_data.size(), input_shape.data(),
      input_shape.size());

  auto output_tensors = session.Run(options, input_names.data(), &input_tensor,
                                    1, output_names.data(), 1);

  return output_tensors;
}
