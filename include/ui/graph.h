#include "raylib.h"
#include "ui/block.h"
#include <memory>
#include <vector>

#pragma once
#ifndef GRAPH_H
#define GRAPH_H

class Graph {
private:
  Block *dragged_block;

  Block *find_block_at(Vector2 cursor_pos);
  Connection find_port_at(Vector2 cursor_pos, PortKind &out_port);
  void draw_grid(const Camera2D &camera);

public:
  std::vector<std::unique_ptr<Block>> blocks;
  std::vector<Block *> orphans;

  std::vector<Block *> roots;
  std::vector<Block *> leafs;

  std::unique_ptr<InputFieldState> input_state;
  bool inference_ran;
  ConnectionState connection_state;
  bool dragging;
  bool topology_dirty;
  Vector2 drag_offset;

  Graph(const size_t shape);

  int count_blocks_with_type(std::string type);
  void push_block(Block *block);
  void remove_block(Block *block);
  void connect(Block *parent, Block *child, const size_t out_port_index,
               const size_t in_port_index);
  void disconnect(Block *parent, Block *child);

  void inference();
  void draw(const Camera2D &camera);
  void update(const Camera2D &camera);
  bool ready();

private:
  void refresh_orphans();
  bool is_reachable_from_root(Block *block);
};

void draw_wire(const Vector2 &from, const Vector2 &to);

void draw_ui(const Graph &graph);

void reset_input_state(InputFieldState &input_state);

#endif
