#include "raylib.h"
#include <memory>
#include <string>
#include <vector>

#pragma once
#ifndef GRAPH_H
#define GRAPH_H

struct InputFieldState;

enum class BlockType { PORT_INPUT, PORT_OUTPUT, RELU };

class Block {
  friend class Graph;

private:
  Vector2 position;
  float width;
  float height;

  float calculate_height();
  Vector2 calculate_input_port();
  Vector2 calculate_output_port();

public:
  BlockType type;
  std::string label;
  std::vector<float> values;
  bool has_results;

  Block(const BlockType &type, const Vector2 &position, const int shape);

  void draw(const InputFieldState &input_state);

  std::vector<std::unique_ptr<Block>> next;
  std::vector<Block *> previous;
};

class Graph {
private:
  Block *dragged_block;

  Block *find_block_at(Vector2 cursor_pos);

public:
  std::unique_ptr<Block> root;
  Block *leaf;
  std::unique_ptr<InputFieldState> input_state;
  bool inference_ran;
  bool dragging;
  Vector2 drag_offset;
  Graph(const int shape); // TODO: Update shape

  void inference();
  void draw();
  bool ready();
  bool update(const Camera2D &camera);
};

struct InputFieldState {
  Block *active_block = nullptr;
  int active_field = -1;
  char buffer[32];
  int cursor = -1;
};

void draw_ui(const Graph &graph);

void reset_input_state(InputFieldState &input_state);

#endif
