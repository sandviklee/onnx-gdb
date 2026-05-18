#pragma once
#include "ir/graph.h"
#include "raylib.h"
#include "ui/block.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ui {

struct Notification {
  std::string msg;
  bool is_error;
  double expire;
};

struct ConnectionState {
  Block *block = nullptr;
  size_t port_index = 0;
  PortKind from_port = PortKind::OUTPUT;
  bool active = false;
};

struct PortRef {
  Block *block = nullptr;
  size_t port_index = 0;
  bool valid() const { return block != nullptr; }
};

class UIGraph {
public:
  ir::Graph ir_graph;

  bool debug_mode = false;
  bool inference_ran = false;

  std::unique_ptr<InputFieldState> input_state;
  ShapePopupState shape_popup;
  ConnectionState connection_state;
  bool dragging = false;
  Vector2 drag_offset = {};
  double last_click_time = 0.0;
  Block *last_click_block = nullptr;

  std::vector<Notification> notifications;

  struct WireTooltip {
    Vector2 world_mid;
    std::string shape_text;
    std::string value_text;
  };
  std::vector<WireTooltip> wire_tooltips;

  explicit UIGraph(size_t initial_shape);

  Block *add_block(const std::string &op_name, const Vector2 &position);
  void remove_block(Block *block);
  void connect(Block *parent, Block *child, size_t out_port_index,
               size_t in_port_index);
  void disconnect(Block *parent, Block *child, size_t out_port_index,
                  size_t in_port_index);
  void clear();
  void rebuild_from_ir();
  void disable_debug();

  void draw(const Camera2D &camera);
  void draw_wire_tooltips(const Camera2D &camera);
  void draw_popup();
  void draw_notifications();
  void push_notification(const std::string &msg, bool is_error);
  void update(const Camera2D &camera);

  bool popup_active() const;
  void open_shape_popup(Block *block);

  Block *find_block_for_node(ir::Node *node) const;

private:
  std::vector<std::unique_ptr<Block>> blocks;
  std::unordered_map<ir::Node *, Block *> node_to_block;
  Block *dragged_block = nullptr;

  Block *find_block_at(Vector2 cursor_pos) const;
  PortRef find_port_at(Vector2 cursor_pos, PortKind &out_port_kind) const;
  void draw_grid(const Camera2D &camera) const;
  void register_block(Block *block);
  void unregister_block(Block *block);
};

void draw_wire(const Vector2 &from, const Vector2 &to);
void reset_input_state(InputFieldState &input_state);

} // namespace ui
