#include <boil/boil.h>
#include <fstream>
#include <iostream>

Boil::Boil() { std::cout << "Boil initialized" << std::endl; };

std::string Boil::serialize(const onnx::ModelProto &model) {
  std::string serialized_model;
  model.SerializeToString(&serialized_model);
  return serialized_model;
}

void Boil::save_model(const onnx::ModelProto &model, const std::string &path) {
  std::ofstream out(path, std::ios::binary);
  model.SerializeToOstream(&out);
}
