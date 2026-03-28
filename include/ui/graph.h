#include "raylib.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#pragma once
#ifndef GRAPH_H
#define GRAPH_H

struct InputFieldState;

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

  // Connections are raw pointers; ownership lives in Graph::blocks.
  std::vector<Block *> next;
  std::vector<Block *> previous;
};

// Tracks an in-progress connection drag from one port.
struct ConnectionState {
  Block *from_block = nullptr;
  PortKind from_port = PortKind::OUTPUT;
  bool active = false;
};

class Graph {
private:
  Block *dragged_block;

  Block *find_block_at(Vector2 cursor_pos);
  Block *find_port_at(Vector2 cursor_pos, PortKind &out_port);

public:
  // Central ownership of ALL blocks.
  std::vector<std::unique_ptr<Block>> blocks;

  Block *root;
  Block *leaf;

  // Blocks not connected to the root DAG.
  std::vector<Block *> orphans;

  std::unique_ptr<InputFieldState> input_state;
  ConnectionState connection_state;
  bool inference_ran;
  bool dragging;
  bool topology_dirty;
  Vector2 drag_offset;

  Graph(const int shape);

  void connect(Block *parent, Block *child);
  void disconnect(Block *parent, Block *child);

  void inference();
  void draw(const Camera2D &camera);
  bool ready();
  bool update(const Camera2D &camera);

private:
  void refresh_orphans();
  bool is_reachable_from_root(Block *block);
};

struct InputFieldState {
  Block *active_block = nullptr;
  int active_field = -1;
  char buffer[32];
  int cursor = -1;
};

void draw_wire(const Vector2 &from, const Vector2 &to);

void draw_ui(const Graph &graph);

void reset_input_state(InputFieldState &input_state);

#endif
