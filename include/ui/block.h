#include "raylib.h"
#include "ui/config.h"
#include <string>
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

enum class BlockType { PORT_INPUT, PORT_OUTPUT, RELU };

enum class PortKind { INPUT, OUTPUT };

class Block {
  friend class Graph;

private:
  float width;
  float height;

  float calculate_height();

public:
  Vector2 position;
  BlockType type;
  std::string label;
  std::vector<float> values;
  bool has_results;

  Block(const BlockType &type, const Vector2 &position, const int shape);

  Vector2 calculate_input_port();
  Vector2 calculate_output_port();

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
