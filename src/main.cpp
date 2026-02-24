#include "boil/boil.h"
#include <iostream>
#include <onnx/onnx_pb.h>
#include <onnxruntime/onnxruntime_cxx_api.h>

onnx::ModelProto create_simple_model() {
  onnx::ModelProto model;
  auto opset = model.add_opset_import();
  opset->set_domain(
      ""); // Standard domain containing all opeartions that we need.
  opset->set_version(13);

  model.set_ir_version(8);
  model.set_producer_name("BlocklyONNX");

  auto *graph = model.mutable_graph();
  graph->set_name("main_graph");

  auto *input = graph->add_input();
  input->set_name("X");
  auto *input_type = input->mutable_type()->mutable_tensor_type();
  input_type->set_elem_type(onnx::TensorProto_DataType_FLOAT);

  auto *node = graph->add_node();
  node->set_op_type("Relu");
  node->add_input("X");
  node->add_output("Y");

  auto *output = graph->add_output();
  output->set_name("Y");

  return model;
}

int main() {
  Boil boil;
  std::string model = boil.serialize(create_simple_model());

  Ort::Env env;
  Ort::Session session(env, model.data(), model.size(),
                       Ort::SessionOptions{nullptr});

  std::vector<float> input_data{-0.5, 0.1, -2,
                                3.5}; // Should give { 0.0, 0.1, 0.0, 3.5 }
  std::vector<int64_t> input_shape{4};

  auto memory_info =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  Ort::Value input_tensor = Ort::Value::CreateTensor(
      memory_info, input_data.data(), input_data.size(), input_shape.data(),
      input_shape.size());

  const char *input_names[]{"X"};
  const char *output_names[]{"Y"};

  Ort::RunOptions run_options{nullptr};
  auto output =
      session.Run(run_options, input_names, &input_tensor, 1, output_names, 1);

  float *output_data = output[0].GetTensorMutableData<float>();

  for (size_t i = 0; i < input_data.size(); ++i) {
    std::cout << "Input: " << input_data[i] << " -> Output: " << output_data[i]
              << std::endl;
  }

  return 0;
}
