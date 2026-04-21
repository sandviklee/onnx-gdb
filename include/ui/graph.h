#include "raylib.h"
#include "ui/block.h"
#include <memory>
#include <string>
#include <vector>

struct Notification {
  std::string msg;
  bool is_error;
  double expire;
};

#pragma once
#ifndef GRAPH_H
#define GRAPH_H

class Graph {
private:
  Block *dragged_block;

  Block *find_block_at(Vector2 cursor_pos);
  Connection find_port_at(Vector2 cursor_pos, PortKind &out_port);
  void draw_grid(const Camera2D &camera);
  int count_blocks_with_type(std::string type);
  bool block_label_exists(const std::string &label);

public:
  std::vector<std::unique_ptr<Block>> blocks;
  std::vector<Block *> orphans;

  std::vector<Block *> roots;
  std::vector<Block *> leafs;

  std::unique_ptr<InputFieldState> input_state;
  ShapePopupState shape_popup;
  bool inference_ran;
  ConnectionState connection_state;
  bool dragging;
  bool topology_dirty;
  Vector2 drag_offset;

  double last_click_time = 0.0;
  Block *last_click_block = nullptr;
  bool debug_mode = false;

  std::vector<Notification> notifications;

  struct WireTooltip {
    Vector2 world_mid;
    std::string shape_text;
    std::string value_text;
  };
  std::vector<WireTooltip> wire_tooltips;

  Graph(const size_t shape);

  std::string generate_block_label(const std::string op);
  void push_block(Block *block);
  void remove_block(Block *block);
  void connect(Block *parent, Block *child, const size_t out_port_index,
               const size_t in_port_index);
  void disconnect(Block *parent, Block *child, const size_t out_port_index,
                  const size_t in_port_index);

  void clear();
  void inference();
  void draw(const Camera2D &camera);
  void draw_wire_tooltips(const Camera2D &camera);
  void draw_popup();
  void draw_notifications();
  void push_notification(const std::string &msg, bool is_error);
  void update(const Camera2D &camera);
  bool ready();
  bool popup_active() const;
  void open_shape_popup(Block *b);

private:
  void refresh_orphans();
  bool is_reachable_from_root(Block *block);
};

void draw_wire(const Vector2 &from, const Vector2 &to);

void draw_ui(const Graph &graph);

void reset_input_state(InputFieldState &input_state);

#endif
