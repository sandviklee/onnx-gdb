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
  std::string label;
  Vector2 position;
  BlockType type;
  float width;
  float height;
  std::vector<float> values;
  bool has_results;

  float calculate_height();
  Vector2 calculate_input_port();
  Vector2 calculate_output_port();

public:
  Block(const BlockType &type, const Vector2 &position, const int shape);

  void draw(const InputFieldState &input_state);

  std::vector<std::unique_ptr<Block>> next;
  std::vector<Block *> previous;
};

class Graph {
private:
  std::unique_ptr<Block> root;
  Block *dragged_block;

  Block *find_block_at(Vector2 cursor_pos);

public:
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
