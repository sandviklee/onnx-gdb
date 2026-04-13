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

enum class BlockType { IO, ACTIVATION, MATH };

struct BlockDefinition {
  std::string name;
  BlockType type;
  size_t num_inputs;
  size_t num_outputs;
};

inline std::unordered_map<std::string, BlockDefinition> BLOCK_REGISTRY = {
    {"MatMul", {"MatMul", BlockType::MATH, 2, 1}},
    {"Add", {"Add", BlockType::MATH, 2, 1}},
    {"Relu", {"Relu", BlockType::ACTIVATION, 1, 1}},
    {"Sigmoid", {"Sigmoid", BlockType::ACTIVATION, 1, 1}},
    {"PortInput", {"PortInput", BlockType::IO, 0, 1}},
    {"PortOutput", {"PortOutput", BlockType::IO, 1, 0}},
};
enum class PortKind { INPUT, OUTPUT };

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
  bool has_results;
  std::string label;

  Block(const std::string &definition, const std::string &label,
        const Vector2 &position, const int shape);

  std::vector<Vector2> calculate_input_ports();
  std::vector<Vector2> calculate_output_ports();

  void draw(const InputFieldState &input_state);

  std::vector<Block *> next;
  std::vector<Block *> previous;
};

struct ConnectionState {
  Block *from_block = nullptr;
  PortKind from_port = PortKind::OUTPUT;
  bool active = false;
};

#endif
