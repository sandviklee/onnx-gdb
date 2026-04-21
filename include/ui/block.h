#include "raylib.h"
#include "ui/config.h"
#include <string>
#include <unordered_map>
#include <vector>

#pragma once
#ifndef BLOCK_H
#define BLOCK_H

class Block;

struct InputFieldState {
  Block *active_block = nullptr;
  int active_field = -1;
  char buffer[32];
  int cursor = -1;
};

struct ShapePopupState {
  Block *target = nullptr;
  int pending_rank = 1; // 0=scalar, 1=vector, 2=matrix, 3=tensor
  int pending_dims[3] = {1, 1, 1};
  bool active = false;
};

enum class BlockType { IO, ACTIVATION, MATH, LAYER };

struct BlockDefinition {
  std::string name;
  BlockType type;
  size_t num_inputs;
  size_t num_outputs;
};

inline std::unordered_map<std::string, BlockDefinition> BLOCK_REGISTRY = {
    // IO
    {"PortInput", {"PortInput", BlockType::IO, 0, 1}},
    {"PortOutput", {"PortOutput", BlockType::IO, 1, 0}},
    // Math
    {"MatMul", {"MatMul", BlockType::MATH, 2, 1}},
    {"Add", {"Add", BlockType::MATH, 2, 1}},
    {"Sub", {"Sub", BlockType::MATH, 2, 1}},
    {"Mul", {"Mul", BlockType::MATH, 2, 1}},
    {"Div", {"Div", BlockType::MATH, 2, 1}},
    {"Pow", {"Pow", BlockType::MATH, 2, 1}},
    {"Gemm", {"Gemm", BlockType::MATH, 3, 1}},
    {"Abs", {"Abs", BlockType::MATH, 1, 1}},
    {"Neg", {"Neg", BlockType::MATH, 1, 1}},
    {"Exp", {"Exp", BlockType::MATH, 1, 1}},
    {"Sqrt", {"Sqrt", BlockType::MATH, 1, 1}},
    {"Log", {"Log", BlockType::MATH, 1, 1}},
    // Activation
    {"Relu", {"Relu", BlockType::ACTIVATION, 1, 1}},
    {"Sigmoid", {"Sigmoid", BlockType::ACTIVATION, 1, 1}},
    {"Tanh", {"Tanh", BlockType::ACTIVATION, 1, 1}},
    {"LeakyRelu", {"LeakyRelu", BlockType::ACTIVATION, 1, 1}},
    {"Elu", {"Elu", BlockType::ACTIVATION, 1, 1}},
    {"Softmax", {"Softmax", BlockType::ACTIVATION, 1, 1}},
    // Layer
    {"Conv", {"Conv", BlockType::LAYER, 2, 1}},
    {"MaxPool", {"MaxPool", BlockType::LAYER, 1, 1}},
    {"AveragePool", {"AveragePool", BlockType::LAYER, 1, 1}},
    {"GlobalAveragePool", {"GlobalAveragePool", BlockType::LAYER, 1, 1}},
    {"BatchNormalization", {"BatchNormalization", BlockType::LAYER, 1, 1}},
    {"Dropout", {"Dropout", BlockType::LAYER, 1, 1}},
    {"Flatten", {"Flatten", BlockType::LAYER, 1, 1}},
    {"Reshape", {"Reshape", BlockType::LAYER, 2, 1}},
    {"Transpose", {"Transpose", BlockType::LAYER, 1, 1}},
    {"Concat", {"Concat", BlockType::LAYER, 2, 1}},
};
enum class PortKind { INPUT, OUTPUT };

struct Connection {
  Block *block;
  size_t port_index;
};

class Block {
  friend class Graph;

private:
  float width;
  float height;

  float calculate_height();

public:
  const BlockDefinition *definition;
  Vector2 position;
  std::vector<float> values;
  std::vector<int> shape_dims; // {} scalar, {N} vec, {R,C} mat, {D,R,C} tensor
  bool has_results;
  std::string label;

  Block(const std::string &name, const std::string &label,
        const Vector2 &position, const int shape);

  std::vector<Vector2> calculate_input_ports();
  std::vector<Vector2> calculate_output_ports();
  std::vector<Rectangle> calculate_field_rects() const;

  void draw(const InputFieldState &input_state);

  std::vector<Connection> inputs;
  std::vector<Connection> outputs;

  // Debug inference results (values flowing out of this block's output)
  std::vector<float> debug_output_values;
  std::vector<int64_t> debug_output_shape;
  bool has_debug_values = false;
};

struct ConnectionState {
  Connection connection;
  PortKind from_port = PortKind::OUTPUT;
  bool active = false;
};

#endif
