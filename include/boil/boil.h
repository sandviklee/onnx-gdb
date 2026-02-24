#include <onnx/onnx_pb.h>
#include <string>
#pragma once
#ifndef BOILPARSER_H
#define BOILPARSER_H

class Boil {
public: // TODO: Define public methods
  Boil();
  std::string serialize(const onnx::ModelProto &model);
  void save_model(const onnx::ModelProto &model, const std::string &path);

private: // TODO: Define private methods
};

#endif
