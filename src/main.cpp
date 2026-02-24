#include "boil/boil.h"
#include <iostream>
#include <onnx/onnx_pb.h>
#include <onnxruntime/onnxruntime_cxx_api.h>

template <typename T>
void print_matrix(const std::vector<std::vector<T>> &matrix) {
  std::cout << "[" << std::endl;
  for (const auto &row : matrix) {
    std::cout << "  [";
    for (size_t i = 0; i < row.size(); i++) {
      std::cout << row[i];
      if (i < row.size() - 1)
        std::cout << ", ";
    }
    std::cout << "]" << std::endl;
  }
  std::cout << "]" << std::endl;
}

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
  std::vector<float> input_data{-0.5, 0.1, -2, 3.5};
  std::vector<int64_t> input_shape{4};
  Boil boil(create_simple_model(), input_data, input_shape, {"X"}, {"Y"});

  Ort::RunOptions run_options{nullptr};
  auto output = boil.run_model(run_options);
  auto info = output[0].GetTensorTypeAndShapeInfo();
  auto shape = info.GetShape();
  std::cout << "Output Shape: " << *shape.data() << std::endl;
  float *data = output[0].GetTensorMutableData<float>();
  int64_t size = *input_shape.data();

  std::vector<std::vector<float>> matrix(shape[0],
                                         std::vector<float>(shape[1]));

  std::cout << "[";
  for (int64_t i = 0; i < size; i++) {
    std::cout << data[i] << (i < size - 1 ? ", " : "");
  }
  std::cout << "]" << std::endl;

  return 0;
}
