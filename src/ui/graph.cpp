#include "ui/graph.h"
#include "raylib.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <sstream>
#include <unordered_set>

namespace ui {

static std::string format_debug_shape(const std::vector<int64_t> &shape) {
  if (shape.empty())
    return "scalar";
  std::string s = "[";
  for (size_t i = 0; i < shape.size(); i++) {
    if (i > 0)
      s += "\xc3\x97";
    s += std::to_string(shape[i]);
  }
  s += "]";
  return s;
}

static std::string format_debug_shape_from_dims(const std::vector<int> &dims) {
  if (dims.empty())
    return "scalar";
  std::string s = "[";
  for (size_t i = 0; i < dims.size(); i++) {
    if (i > 0)
      s += "\xc3\x97";
    s += std::to_string(dims[i]);
  }
  s += "]";
  return s;
}

static std::string format_debug_values(const std::vector<float> &vals) {
  if (vals.empty())
    return "";
  char buf[32];
  std::string s;
  size_t show = std::min(vals.size(), (size_t)4);
  for (size_t i = 0; i < show; i++) {
    if (i > 0)
      s += ", ";
    snprintf(buf, sizeof(buf), "%.3g", vals[i]);
    s += buf;
  }
  if (vals.size() > 4)
    s += " \xe2\x80\xa6";
  return s;
}

UIGraph::UIGraph(size_t initial_shape)
    : input_state(std::make_unique<InputFieldState>()) {
  auto *input_node =
      new ir::Node("PortInput", ir_graph.generate_node_label("PortInput"));
  input_node->shape_dims = {(int)initial_shape};
  input_node->values.assign(initial_shape, 0.0f);
  input_node->layout_hint = {200.0f, 200.0f};

  auto *output_node =
      new ir::Node("PortOutput", ir_graph.generate_node_label("PortOutput"));
  output_node->layout_hint = {800.0f, 200.0f};

  auto *relu_node = new ir::Node("Relu", ir_graph.generate_node_label("Relu"));
  relu_node->layout_hint = {450.0f, 400.0f};

  ir_graph.add_node(input_node);
  ir_graph.add_node(output_node);
  ir_graph.add_node(relu_node);

  ir_graph.connect(input_node, relu_node, 0, 0);
  ir_graph.connect(relu_node, output_node, 0, 0);

  for (const auto &node_ptr : ir_graph.nodes) {
    ir::Node *node = node_ptr.get();
    Vector2 pos = {node->layout_hint.x, node->layout_hint.y};
    auto block = std::make_unique<Block>(node, pos);
    register_block(block.get());
    blocks.push_back(std::move(block));
  }
}

void UIGraph::register_block(Block *block) {
  node_to_block[block->node] = block;
}

void UIGraph::unregister_block(Block *block) {
  node_to_block.erase(block->node);
}

Block *UIGraph::find_block_for_node(ir::Node *node) const {
  auto it = node_to_block.find(node);
  return it != node_to_block.end() ? it->second : nullptr;
}

Block *UIGraph::add_block(const std::string &op_name, const Vector2 &position) {
  std::string label = ir_graph.generate_node_label(op_name);
  auto *node = new ir::Node(op_name, label);
  node->layout_hint = {position.x, position.y};
  if (op_name == "PortInput") {
    node->shape_dims = {1};
    node->values = {0.0f};
  }
  ir_graph.add_node(node);
  auto block = std::make_unique<Block>(node, position);
  block->update_height();
  Block *block_ptr = block.get();
  register_block(block_ptr);
  blocks.push_back(std::move(block));
  return block_ptr;
}

void UIGraph::remove_block(Block *block) {
  if (input_state->active_node == block->node)
    reset_input_state(*input_state);
  if (last_click_block == block)
    last_click_block = nullptr;

  unregister_block(block);
  ir_graph.remove_node(block->node);

  blocks.erase(std::remove_if(blocks.begin(), blocks.end(),
                              [block](const std::unique_ptr<Block> &b) {
                                return b.get() == block;
                              }),
               blocks.end());
}

void UIGraph::connect(Block *parent, Block *child, size_t out_port_index,
                      size_t in_port_index) {
  ir_graph.connect(parent->node, child->node, out_port_index, in_port_index);
}

void UIGraph::disconnect(Block *parent, Block *child, size_t out_port_index,
                         size_t in_port_index) {
  ir_graph.disconnect(parent->node, child->node, out_port_index, in_port_index);
}

void UIGraph::clear() {
  reset_input_state(*input_state);
  blocks.clear();
  node_to_block.clear();
  ir_graph.clear();
  dragging = false;
  dragged_block = nullptr;
  last_click_block = nullptr;
  connection_state = {};
  shape_popup = {};
  attr_popup = {};
  inference_ran = false;
}

void UIGraph::rebuild_from_ir() {
  reset_input_state(*input_state);
  blocks.clear();
  node_to_block.clear();
  dragging = false;
  dragged_block = nullptr;
  last_click_block = nullptr;
  connection_state = {};
  shape_popup = {};
  attr_popup = {};

  for (const auto &node_ptr : ir_graph.nodes) {
    ir::Node *node = node_ptr.get();
    Vector2 pos = {node->layout_hint.x, node->layout_hint.y};
    auto block = std::make_unique<Block>(node, pos);
    block->update_height();
    register_block(block.get());
    blocks.push_back(std::move(block));
  }
}

void UIGraph::disable_debug() {
  debug_mode = false;
  for (const auto &node_ptr : ir_graph.nodes) {
    node_ptr->has_debug_values = false;
  }
}

void UIGraph::push_notification(const std::string &msg, bool is_error) {
  notifications.push_back({msg, is_error, GetTime() + 5.0});
}

bool UIGraph::popup_active() const {
  return shape_popup.active || attr_popup.active;
}

void UIGraph::open_shape_popup(Block *block) {
  ir::Node *node = block->node;
  shape_popup.target = node;
  shape_popup.pending_rank = (int)node->shape_dims.size();
  shape_popup.pending_dims[0] =
      node->shape_dims.size() >= 1 ? node->shape_dims[0] : 1;
  shape_popup.pending_dims[1] =
      node->shape_dims.size() >= 2 ? node->shape_dims[1] : 1;
  shape_popup.pending_dims[2] =
      node->shape_dims.size() >= 3 ? node->shape_dims[2] : 1;
  shape_popup.pending_dims[3] =
      node->shape_dims.size() >= 4 ? node->shape_dims[3] : 1;
  shape_popup.pending_is_initializer = node->is_initializer;
  shape_popup.active = true;
}

static std::string attr_to_string(const ir::AttributeValue &av) {
  char buf[64];
  switch (av.type) {
  case ir::AttrType::INT:
    snprintf(buf, sizeof(buf), "%lld", (long long)av.i);
    return buf;
  case ir::AttrType::FLOAT:
    snprintf(buf, sizeof(buf), "%g", av.f);
    return buf;
  case ir::AttrType::INTS: {
    std::string s;
    for (size_t i = 0; i < av.ints.size(); i++) {
      if (i)
        s += ",";
      snprintf(buf, sizeof(buf), "%lld", (long long)av.ints[i]);
      s += buf;
    }
    return s;
  }
  case ir::AttrType::FLOATS: {
    std::string s;
    for (size_t i = 0; i < av.floats.size(); i++) {
      if (i)
        s += ",";
      snprintf(buf, sizeof(buf), "%g", av.floats[i]);
      s += buf;
    }
    return s;
  }
  case ir::AttrType::STRING:
    return av.s;
  }
  return "";
}

static void parse_into_attr(ir::AttributeValue &av, const std::string &buf) {
  switch (av.type) {
  case ir::AttrType::INT:
    av.i = (int64_t)atoll(buf.c_str());
    break;
  case ir::AttrType::FLOAT:
    av.f = (float)atof(buf.c_str());
    break;
  case ir::AttrType::INTS: {
    av.ints.clear();
    std::stringstream ss(buf);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
      if (!tok.empty())
        av.ints.push_back((int64_t)atoll(tok.c_str()));
    }
    break;
  }
  case ir::AttrType::FLOATS: {
    av.floats.clear();
    std::stringstream ss(buf);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
      if (!tok.empty())
        av.floats.push_back((float)atof(tok.c_str()));
    }
    break;
  }
  case ir::AttrType::STRING:
    av.s = buf;
    break;
  }
}

void UIGraph::open_attr_popup(Block *block) {
  ir::Node *node = block->node;
  attr_popup = {};
  attr_popup.target = node;
  for (const auto &spec : node->spec->attributes) {
    auto it = node->attributes.find(spec.name);
    const ir::AttributeValue &av =
        it != node->attributes.end() ? it->second : spec.default_value;
    attr_popup.names.push_back(spec.name);
    attr_popup.buffers.push_back(attr_to_string(av));
  }
  attr_popup.active = !attr_popup.names.empty();
}

void UIGraph::draw_attr_popup() {
  if (!attr_popup.active)
    return;

  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  DrawRectangle(0, 0, sw, sh, {0, 0, 0, 120});

  size_t n = attr_popup.names.size();
  const float row_h = 32.0f;
  const float pw = 360.0f;
  float ph = 110.0f + (float)n * row_h + 70.0f;
  float px = (sw - pw) / 2.0f;
  float py = (sh - ph) / 2.0f;

  DrawRectangleRec({px, py, pw, ph}, {35, 35, 40, 255});
  DrawRectangleLinesEx({px, py, pw, ph}, 1.5f, {70, 70, 75, 255});
  DrawRectangleRec({px, py, pw, 40.0f}, {25, 25, 30, 255});
  std::string title = "Attributes: " + attr_popup.target->spec->name;
  DrawText(title.c_str(), px + 12, py + 12, 15, WHITE);

  Vector2 mouse = GetMousePosition();
  float row_y = py + 60.0f;
  const float label_w = 130.0f;

  for (size_t i = 0; i < n; i++) {
    DrawText(attr_popup.names[i].c_str(), px + 14, row_y + 6, 13, LIGHTGRAY);
    Rectangle field = {px + 14 + label_w, row_y, pw - 28 - label_w, 24.0f};
    bool focused = attr_popup.active_field == (int)i;
    DrawRectangleRec(field, focused ? Color{55, 55, 70, 255}
                                    : Color{50, 50, 58, 255});
    DrawRectangleLinesEx(field, 1.0f,
                         focused ? Color{120, 160, 220, 255}
                                 : Color{80, 80, 90, 255});
    DrawText(attr_popup.buffers[i].c_str(), field.x + 6, field.y + 4, 13,
             WHITE);
    if (focused && ((int)(GetTime() * 2)) % 2 == 0) {
      int cur_x = field.x + 6 +
                  MeasureText(attr_popup.buffers[i].c_str(), 13);
      DrawLine(cur_x, field.y + 4, cur_x, field.y + field.height - 4, RED);
    }
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
        CheckCollisionPointRec(mouse, field))
      attr_popup.active_field = (int)i;
    row_y += row_h;
  }

  float bw = 90.0f, bh = 32.0f;
  float by = py + ph - 42.0f;
  Rectangle cancel_r = {px + 20, by, bw, bh};
  Rectangle ok_r = {px + pw - 20 - bw, by, bw, bh};
  bool cancel_hov = CheckCollisionPointRec(mouse, cancel_r);
  bool ok_hov = CheckCollisionPointRec(mouse, ok_r);

  DrawRectangleRec(cancel_r, cancel_hov ? Color{110, 55, 55, 255}
                                        : Color{80, 40, 40, 255});
  DrawRectangleLinesEx(cancel_r, 1.0f, {120, 60, 60, 255});
  DrawText("Cancel", cancel_r.x + (bw - MeasureText("Cancel", 14)) / 2,
           by + 9, 14, WHITE);
  DrawRectangleRec(ok_r,
                   ok_hov ? Color{55, 110, 55, 255} : Color{40, 80, 40, 255});
  DrawRectangleLinesEx(ok_r, 1.0f, {60, 120, 60, 255});
  DrawText("OK", ok_r.x + (bw - MeasureText("OK", 14)) / 2, by + 9, 14, WHITE);

  if (attr_popup.active_field >= 0) {
    int key = GetCharPressed();
    while (key > 0) {
      if ((key >= '0' && key <= '9') || key == '-' || key == '.' ||
          key == ',') {
        attr_popup.buffers[attr_popup.active_field].push_back((char)key);
      }
      key = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
      auto &b = attr_popup.buffers[attr_popup.active_field];
      if (!b.empty())
        b.pop_back();
    }
    if (IsKeyPressed(KEY_TAB)) {
      attr_popup.active_field = (attr_popup.active_field + 1) % (int)n;
    }
  }

  auto commit = [&]() {
    ir::Node *target = attr_popup.target;
    for (size_t i = 0; i < n; i++) {
      auto it = target->attributes.find(attr_popup.names[i]);
      if (it == target->attributes.end())
        continue;
      parse_into_attr(it->second, attr_popup.buffers[i]);
    }
    ir_graph.topology_dirty = true;
  };

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    if (ok_hov) {
      commit();
      attr_popup = {};
    } else if (cancel_hov) {
      attr_popup = {};
    }
  }

  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
    commit();
    attr_popup = {};
  }
  if (IsKeyPressed(KEY_ESCAPE)) {
    attr_popup = {};
  }
}

Block *UIGraph::find_block_at(Vector2 cursor_pos) const {
  for (const auto &block_ptr : blocks) {
    Block *block = block_ptr.get();
    Rectangle block_rect = {block->position.x, block->position.y, block->width,
                            block->height};
    if (CheckCollisionPointRec(cursor_pos, block_rect))
      return block;
  }
  return nullptr;
}

PortRef UIGraph::find_port_at(Vector2 cursor_pos,
                              PortKind &out_port_kind) const {
  for (const auto &block_ptr : blocks) {
    Block *block = block_ptr.get();

    if (block->node->spec->num_outputs > 0) {
      auto out_ports = block->calculate_output_ports();
      for (size_t i = 0; i < out_ports.size(); i++) {
        float dx = cursor_pos.x - out_ports[i].x;
        float dy = cursor_pos.y - out_ports[i].y;
        if (dx * dx + dy * dy <= PORT_RADIUS * PORT_RADIUS) {
          out_port_kind = PortKind::OUTPUT;
          return {block, i};
        }
      }
    }

    if (block->node->spec->num_inputs > 0) {
      auto in_ports = block->calculate_input_ports();
      for (size_t i = 0; i < in_ports.size(); i++) {
        float dx = cursor_pos.x - in_ports[i].x;
        float dy = cursor_pos.y - in_ports[i].y;
        if (dx * dx + dy * dy <= PORT_RADIUS * PORT_RADIUS) {
          out_port_kind = PortKind::INPUT;
          return {block, i};
        }
      }
    }
  }
  return {};
}

void UIGraph::draw_grid(const Camera2D &camera) const {
  float grid_spacing = 150.0f;
  float inv_zoom = 1.0f / camera.zoom;
  float half_w = GetScreenWidth() * inv_zoom;
  float half_h = GetScreenHeight() * inv_zoom;

  float start_x =
      floorf((camera.target.x - half_w) / grid_spacing) * grid_spacing;
  float start_y =
      floorf((camera.target.y - half_h) / grid_spacing) * grid_spacing;
  float end_x = camera.target.x + half_w;
  float end_y = camera.target.y + half_h;

  for (float x = start_x; x <= end_x; x += grid_spacing)
    DrawLineV({x, start_y}, {x, end_y}, LIGHTGRAY);
  for (float y = start_y; y <= end_y; y += grid_spacing)
    DrawLineV({start_x, y}, {end_x, y}, LIGHTGRAY);
}

void UIGraph::draw(const Camera2D &camera) {
  draw_grid(camera);
  wire_tooltips.clear();

  auto collect_wire_tooltip = [&](ir::Node *src, Vector2 from, Vector2 to) {
    if (!debug_mode)
      return;
    Vector2 mid = {(from.x + to.x) * 0.5f, (from.y + to.y) * 0.5f};
    std::string shape_text, value_text;
    if (src->spec->name == "PortInput") {
      shape_text = format_debug_shape_from_dims(src->shape_dims);
      value_text = format_debug_values(src->values);
    } else if (src->has_debug_values) {
      shape_text = format_debug_shape(src->debug_output_shape);
      value_text = format_debug_values(src->debug_output_values);
    }
    if (!shape_text.empty())
      wire_tooltips.push_back({mid, shape_text, value_text});
  };

  auto draw_node = [&](ir::Node *node) {
    Block *block = find_block_for_node(node);
    if (!block)
      return;
    block->update_height();
    block->draw(*input_state);

    for (const auto &edge : node->outputs) {
      Block *child_block = find_block_for_node(edge.node);
      if (!child_block)
        continue;

      size_t in_port_index = 0;
      for (const auto &in_edge : edge.node->inputs) {
        if (in_edge.node == node) {
          in_port_index = in_edge.port_index;
          break;
        }
      }

      Vector2 from = block->calculate_output_ports()[edge.port_index];
      Vector2 to = child_block->calculate_input_ports()[in_port_index];
      draw_wire(from, to);
      collect_wire_tooltip(node, from, to);
    }
  };

  if (!ir_graph.roots.empty()) {
    std::deque<ir::Node *> queue(ir_graph.roots.begin(), ir_graph.roots.end());
    std::unordered_set<ir::Node *> visited(ir_graph.roots.begin(),
                                           ir_graph.roots.end());
    while (!queue.empty()) {
      ir::Node *current = queue.front();
      queue.pop_front();
      draw_node(current);
      for (const auto &edge : current->outputs) {
        if (visited.insert(edge.node).second)
          queue.push_back(edge.node);
      }
    }
  }

  for (ir::Node *orphan : ir_graph.orphans) {
    draw_node(orphan);
  }

  if (connection_state.active && connection_state.block) {
    Vector2 from;
    if (connection_state.from_port == PortKind::OUTPUT) {
      from = connection_state.block
                 ->calculate_output_ports()[connection_state.port_index];
    } else {
      from = connection_state.block
                 ->calculate_input_ports()[connection_state.port_index];
    }
    Vector2 mouse_world = GetScreenToWorld2D(GetMousePosition(), camera);
    draw_wire(from, mouse_world);
  }
}

static std::vector<std::string> split_words(const std::string &s) {
  std::istringstream iss(s);
  return {std::istream_iterator<std::string>(iss),
          std::istream_iterator<std::string>()};
}

void UIGraph::draw_notifications() {
  double now = GetTime();
  notifications.erase(
      std::remove_if(notifications.begin(), notifications.end(),
                     [now](const Notification &n) { return now > n.expire; }),
      notifications.end());

  int sw = GetScreenWidth();
  float y = 70.0f;
  for (const auto &n : notifications) {
    float alpha_f = (float)std::min(1.0, n.expire - now);
    unsigned char alpha = (unsigned char)(alpha_f * 220);
    Color bg =
        n.is_error ? Color{170, 45, 45, alpha} : Color{45, 140, 71, alpha};
    Color fg = {255, 255, 255, alpha};

    float h = 36.0f;
    size_t words_per_line = 4;
    std::string message;
    auto words = split_words(n.msg);
    for (size_t i = 0; i < words.size(); i++) {
      if (i % words_per_line == 0 && i != 0) {
        message += "\n";
        h += 20.0f;
      }
      message += words[i] + " ";
    }
    int tw = MeasureText(message.c_str(), 14);
    float w = tw + 36.0f;
    float x = (sw - w) / 2.0f;
    DrawRectangleRounded({x, y, w, h}, 0.2f, 6, bg);
    DrawRectangleRoundedLinesEx({x, y, w, h}, 0.2f, 6, 1.0f,
                                {255, 255, 255, (unsigned char)(alpha / 2)});
    DrawText(message.c_str(), x + (w - tw) / 2.0f, y + 14, 14, fg);
    y += h + 6.0f;
  }
}

static void draw_dim_stepper(const char *label_str, int value, float lx,
                             float ly, float /*total_w*/, Vector2 mouse,
                             Rectangle &out_minus, Rectangle &out_plus) {
  const float btn_w = 26.0f, btn_h = 26.0f, cnt_w = 40.0f;
  int lbl_w = MeasureText(label_str, 13);
  DrawText(label_str, lx, ly + 6, 13, LIGHTGRAY);

  float sx = lx + lbl_w + 8;
  out_minus = {sx, ly, btn_w, btn_h};
  Rectangle cnt_r = {sx + btn_w + 2, ly, cnt_w, btn_h};
  out_plus = {sx + btn_w + cnt_w + 4, ly, btn_w, btn_h};

  bool mhov = CheckCollisionPointRec(mouse, out_minus);
  bool phov = CheckCollisionPointRec(mouse, out_plus);

  DrawRectangleRec(out_minus,
                   mhov ? Color{100, 100, 110, 255} : Color{65, 65, 75, 255});
  DrawRectangleLinesEx(out_minus, 1.0f, {85, 85, 95, 255});
  DrawText("-", out_minus.x + (btn_w - MeasureText("-", 16)) / 2, ly + 4, 16,
           WHITE);

  DrawRectangleRec(cnt_r, {50, 50, 58, 255});
  DrawRectangleLinesEx(cnt_r, 1.0f, {75, 75, 85, 255});
  char vs[8];
  snprintf(vs, sizeof(vs), "%d", value);
  DrawText(vs, cnt_r.x + (cnt_w - MeasureText(vs, 14)) / 2, ly + 5, 14, WHITE);

  DrawRectangleRec(out_plus,
                   phov ? Color{100, 100, 110, 255} : Color{65, 65, 75, 255});
  DrawRectangleLinesEx(out_plus, 1.0f, {85, 85, 95, 255});
  DrawText("+", out_plus.x + (btn_w - MeasureText("+", 16)) / 2, ly + 4, 16,
           WHITE);
}

void UIGraph::draw_popup() {
  if (!shape_popup.active)
    return;

  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  DrawRectangle(0, 0, sw, sh, {0, 0, 0, 120});

  const float pw = 320.0f, ph = 400.0f;
  float px = (sw - pw) / 2.0f;
  float py = (sh - ph) / 2.0f;

  DrawRectangleRec({px, py, pw, ph}, {35, 35, 40, 255});
  DrawRectangleLinesEx({px, py, pw, ph}, 1.5f, {70, 70, 75, 255});
  DrawRectangleRec({px, py, pw, 40.0f}, {25, 25, 30, 255});
  DrawText("Configure Input Shape", px + 12, py + 12, 15, WHITE);

  Vector2 mouse = GetMousePosition();
  std::string block_lbl = "Block: " + shape_popup.target->label;
  DrawText(block_lbl.c_str(), px + 14, py + 50, 14, LIGHTGRAY);

  DrawText("Rank:", px + 14, py + 74, 13, LIGHTGRAY);
  const char *rank_labels[] = {"scalar", "vector", "matrix", "tensor3",
                               "tensor4"};
  const float rbw = 56.0f, rbh = 26.0f, rbgap = 4.0f;
  float rx0 = px + 14;
  for (int r = 0; r < 5; r++) {
    Rectangle rb = {rx0 + r * (rbw + rbgap), py + 90, rbw, rbh};
    bool sel = (shape_popup.pending_rank == r);
    bool hov = CheckCollisionPointRec(mouse, rb);
    Color bg = sel ? Color{70, 120, 190, 255}
                   : (hov ? Color{65, 65, 75, 255} : Color{50, 50, 58, 255});
    DrawRectangleRec(rb, bg);
    DrawRectangleLinesEx(
        rb, 1.0f, sel ? Color{100, 160, 230, 255} : Color{80, 80, 90, 255});
    int tw = MeasureText(rank_labels[r], 13);
    DrawText(rank_labels[r], rb.x + (rbw - tw) / 2, rb.y + 6, 13, WHITE);
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && hov)
      shape_popup.pending_rank = r;
  }

  DrawLine(px + 12, py + 126, px + pw - 12, py + 126, {65, 65, 72, 255});
  DrawText("Dimensions:", px + 14, py + 132, 13, LIGHTGRAY);

  int rank = shape_popup.pending_rank;
  float dim_y_start = py + 152.0f;
  const float row_h = 34.0f;
  const char *dim_names_r1[] = {"D:"};
  const char *dim_names_r2[] = {"R:", "C:"};
  const char *dim_names_r3[] = {"D:", "R:", "C:"};
  const char *dim_names_r4[] = {"N:", "C:", "H:", "W:"};
  const char *const *dim_names = dim_names_r3;
  if (rank == 1)
    dim_names = dim_names_r1;
  else if (rank == 2)
    dim_names = dim_names_r2;
  else if (rank == 4)
    dim_names = dim_names_r4;

  Rectangle minus_r[4], plus_r[4];
  if (rank == 0) {
    DrawText("1 value (no dimensions)", px + 14, dim_y_start + 4, 13, GRAY);
  } else {
    for (int d = 0; d < rank; d++) {
      draw_dim_stepper(dim_names[d], shape_popup.pending_dims[d], px + 14,
                       dim_y_start + d * row_h, pw - 28, mouse, minus_r[d],
                       plus_r[d]);
    }
  }

  int total = 1;
  for (int d = 0; d < rank; d++)
    total *= shape_popup.pending_dims[d];
  char total_str[64];
  snprintf(total_str, sizeof(total_str), "Total: %d value%s", total,
           total == 1 ? "" : "s");
  DrawText(total_str, px + 14, py + ph - 60, 13,
           total > 1000 ? Color{200, 80, 80, 255} : LIGHTGRAY);

  Rectangle init_box = {px + 14, py + ph - 100, 18.0f, 18.0f};
  bool init_hov = CheckCollisionPointRec(mouse, init_box);
  DrawRectangleRec(init_box, shape_popup.pending_is_initializer
                                 ? Color{70, 120, 190, 255}
                                 : Color{50, 50, 58, 255});
  DrawRectangleLinesEx(init_box, 1.0f,
                       init_hov ? Color{120, 160, 220, 255}
                                : Color{90, 90, 100, 255});
  if (shape_popup.pending_is_initializer)
    DrawText("x", init_box.x + 4, init_box.y + 1, 16, WHITE);
  DrawText("Export as weight (initializer)", px + 40, py + ph - 98, 13,
           LIGHTGRAY);
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && init_hov)
    shape_popup.pending_is_initializer = !shape_popup.pending_is_initializer;

  float bw = 90.0f, bh = 32.0f;
  float by = py + ph - 42.0f;
  Rectangle cancel_r = {px + 20, by, bw, bh};
  Rectangle ok_r = {px + pw - 20 - bw, by, bw, bh};

  bool cancel_hov = CheckCollisionPointRec(mouse, cancel_r);
  bool ok_hov = CheckCollisionPointRec(mouse, ok_r);

  DrawRectangleRec(cancel_r, cancel_hov ? Color{110, 55, 55, 255}
                                        : Color{80, 40, 40, 255});
  DrawRectangleLinesEx(cancel_r, 1.0f, {120, 60, 60, 255});
  DrawText("Cancel", cancel_r.x + (bw - MeasureText("Cancel", 14)) / 2, by + 9,
           14, WHITE);

  DrawRectangleRec(ok_r,
                   ok_hov ? Color{55, 110, 55, 255} : Color{40, 80, 40, 255});
  DrawRectangleLinesEx(ok_r, 1.0f, {60, 120, 60, 255});
  DrawText("OK", ok_r.x + (bw - MeasureText("OK", 14)) / 2, by + 9, 14, WHITE);

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    for (int d = 0; d < rank; d++) {
      if (CheckCollisionPointRec(mouse, minus_r[d]) &&
          shape_popup.pending_dims[d] > 1)
        shape_popup.pending_dims[d]--;
      if (CheckCollisionPointRec(mouse, plus_r[d]) &&
          shape_popup.pending_dims[d] < 128)
        shape_popup.pending_dims[d]++;
    }
    if (ok_hov) {
      ir::Node *target = shape_popup.target;
      std::vector<int> new_dims;
      for (int d = 0; d < rank; d++)
        new_dims.push_back(shape_popup.pending_dims[d]);
      int new_total = rank == 0 ? 1 : total;
      target->shape_dims = new_dims;
      target->values.resize(new_total, 0.0f);
      target->is_initializer = shape_popup.pending_is_initializer;
      reset_input_state(*input_state);
      shape_popup.active = false;
      shape_popup.target = nullptr;
    } else if (cancel_hov) {
      shape_popup.active = false;
      shape_popup.target = nullptr;
    }
  }

  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
    ir::Node *target = shape_popup.target;
    std::vector<int> new_dims;
    for (int d = 0; d < rank; d++)
      new_dims.push_back(shape_popup.pending_dims[d]);
    int new_total = rank == 0 ? 1 : total;
    target->shape_dims = new_dims;
    target->values.resize(new_total, 0.0f);
    target->is_initializer = shape_popup.pending_is_initializer;
    reset_input_state(*input_state);
    shape_popup.active = false;
    shape_popup.target = nullptr;
  }

  if (IsKeyPressed(KEY_ESCAPE)) {
    shape_popup.active = false;
    shape_popup.target = nullptr;
  }
}

void UIGraph::draw_wire_tooltips(const Camera2D &camera) {
  const int fs = 12;
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();

  for (const auto &tt : wire_tooltips) {
    Vector2 sp = GetWorldToScreen2D(tt.world_mid, camera);

    int w1 = MeasureText(tt.shape_text.c_str(), fs);
    int w2 = tt.value_text.empty() ? 0 : MeasureText(tt.value_text.c_str(), fs);
    float content_w = (float)std::max(w1, w2);
    bool two_lines = !tt.value_text.empty();
    float box_w = content_w + 16.0f;
    float box_h = two_lines ? (float)(fs * 2 + 14) : (float)(fs + 10);

    float bx = sp.x - box_w * 0.5f;
    float by = sp.y - box_h * 0.5f;
    bx = std::max(4.0f, std::min(bx, (float)(sw)-box_w - 4.0f));
    by = std::max(4.0f, std::min(by, (float)(sh)-box_h - 4.0f));

    Rectangle box = {bx, by, box_w, box_h};
    DrawRectangleRounded(box, 0.35f, 6, {18, 18, 28, 225});
    DrawRectangleRoundedLinesEx(box, 0.35f, 6, 1.0f, {90, 110, 160, 200});

    DrawText(tt.shape_text.c_str(), (int)(bx + 8), (int)(by + 4), fs,
             {160, 200, 255, 240});
    if (two_lines) {
      DrawText(tt.value_text.c_str(), (int)(bx + 8), (int)(by + 4 + fs + 2), fs,
               {200, 200, 200, 220});
    }
  }
}

void UIGraph::update(const Camera2D &camera) {
  if (shape_popup.active || attr_popup.active)
    return;

  Vector2 mouse_screen = GetMousePosition();
  Vector2 mouse_world = GetScreenToWorld2D(mouse_screen, camera);

  PortKind kind;
  Block *hovered_block = find_block_at(mouse_world);
  PortRef hovered_port = find_port_at(mouse_world, kind);
  if (hovered_block != nullptr) {
    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
  } else if (!hovered_port.valid()) {
    SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
  } else {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
  }

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    if (input_state->active_node != nullptr)
      reset_input_state(*input_state);

    PortKind clicked_port_kind;
    PortRef port_ref = find_port_at(mouse_world, clicked_port_kind);
    if (port_ref.valid()) {
      connection_state.block = port_ref.block;
      connection_state.port_index = port_ref.port_index;
      connection_state.from_port = clicked_port_kind;
      connection_state.active = true;
      return;
    }

    Block *clicked = find_block_at(mouse_world);
    if (!clicked)
      return;

    if (clicked->node->spec->name == "PortInput") {
      Rectangle header_rect = {clicked->position.x, clicked->position.y,
                               clicked->width, IO_FIELD_START_H};
      if (CheckCollisionPointRec(mouse_world, header_rect)) {
        double now = GetTime();
        if (now - last_click_time < 0.35 && last_click_block == clicked) {
          open_shape_popup(clicked);
          last_click_block = nullptr;
          return;
        }
        last_click_time = now;
        last_click_block = clicked;
      }
    } else if (clicked->node->spec->name != "PortOutput" &&
               !clicked->node->spec->attributes.empty()) {
      double now = GetTime();
      if (now - last_click_time < 0.35 && last_click_block == clicked) {
        open_attr_popup(clicked);
        last_click_block = nullptr;
        return;
      }
      last_click_time = now;
      last_click_block = clicked;
    }

    bool clicked_field = false;
    auto field_rects = clicked->calculate_field_rects();
    for (size_t fi = 0;
         fi < field_rects.size() && fi < clicked->node->values.size(); fi++) {
      if (CheckCollisionPointRec(mouse_world, field_rects[fi])) {
        if (input_state->active_node != nullptr &&
            input_state->active_field >= 0) {
          float val = (float)atof(input_state->buffer);
          input_state->active_node->values[input_state->active_field] = val;
        }
        input_state->active_node = clicked->node;
        input_state->active_field = (int)fi;
        snprintf(input_state->buffer, sizeof(input_state->buffer), "%.2f",
                 clicked->node->values[fi]);
        input_state->cursor = (int)strlen(input_state->buffer);
        clicked_field = true;
        break;
      }
    }

    dragging = true;
    dragged_block = clicked;
    drag_offset = {mouse_world.x - clicked->position.x,
                   mouse_world.y - clicked->position.y};

    if (!clicked_field && input_state->active_node != nullptr) {
      float val = (float)atof(input_state->buffer);
      input_state->active_node->values[input_state->active_field] = val;
      reset_input_state(*input_state);
    }
  }

  if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
    Block *clicked = find_block_at(mouse_world);
    if (clicked)
      remove_block(clicked);

    PortKind clicked_port_kind;
    PortRef port_ref = find_port_at(mouse_world, clicked_port_kind);
    if (port_ref.valid()) {
      if (clicked_port_kind == PortKind::OUTPUT) {
        auto outputs = port_ref.block->node->outputs;
        for (const auto &edge : outputs) {
          Block *child_block = find_block_for_node(edge.node);
          if (child_block)
            disconnect(port_ref.block, child_block, edge.port_index,
                       port_ref.port_index);
        }
      } else {
        auto inputs = port_ref.block->node->inputs;
        for (const auto &edge : inputs) {
          Block *parent_block = find_block_for_node(edge.node);
          if (parent_block)
            disconnect(parent_block, port_ref.block, edge.port_index,
                       port_ref.port_index);
        }
      }
    }
  }

  if (connection_state.active && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    PortKind target_port_kind;
    PortRef target = find_port_at(mouse_world, target_port_kind);

    if (target.valid() && target.block != connection_state.block) {
      if (connection_state.from_port == PortKind::OUTPUT &&
          target_port_kind == PortKind::INPUT) {
        connect(connection_state.block, target.block,
                connection_state.port_index, target.port_index);
      } else if (connection_state.from_port == PortKind::INPUT &&
                 target_port_kind == PortKind::OUTPUT) {
        connect(target.block, connection_state.block, target.port_index,
                connection_state.port_index);
      }
    }
    connection_state.active = false;
    connection_state.block = nullptr;
  }

  if (input_state->active_node != nullptr) {
    int key = GetCharPressed();
    while (key > 0) {
      if ((key >= '0' && key <= '9') || key == '-' || key == '.') {
        int len = (int)strlen(input_state->buffer);
        if (len < 30) {
          input_state->buffer[len] = (char)key;
          input_state->buffer[len + 1] = '\0';
          input_state->cursor++;
        }
      }
      key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
      int len = (int)strlen(input_state->buffer);
      if (len > 0) {
        input_state->buffer[len - 1] = '\0';
        input_state->cursor--;
      }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
      float val = (float)atof(input_state->buffer);
      input_state->active_node->values[input_state->active_field] = val;
      reset_input_state(*input_state);
    }

    if (IsKeyPressed(KEY_TAB) && IsKeyDown(KEY_LEFT_SHIFT)) {
      float val = (float)atof(input_state->buffer);
      input_state->active_node->values[input_state->active_field] = val;
      int previous_field = input_state->active_field - 1;
      if (previous_field >= 0) {
        input_state->active_field = previous_field;
        snprintf(input_state->buffer, sizeof(input_state->buffer), "%.2f",
                 input_state->active_node->values[previous_field]);
        input_state->cursor = (int)strlen(input_state->buffer);
      } else {
        reset_input_state(*input_state);
      }
    } else if (IsKeyPressed(KEY_TAB)) {
      Block *active_block = find_block_for_node(input_state->active_node);
      float val = (float)atof(input_state->buffer);
      input_state->active_node->values[input_state->active_field] = val;
      int next_field = input_state->active_field + 1;
      if (active_block &&
          next_field < (int)active_block->calculate_field_rects().size()) {
        input_state->active_field = next_field;
        snprintf(input_state->buffer, sizeof(input_state->buffer), "%.2f",
                 input_state->active_node->values[next_field]);
        input_state->cursor = (int)strlen(input_state->buffer);
      } else {
        reset_input_state(*input_state);
      }
    }
  }

  if (dragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    dragged_block->position.x = mouse_world.x - drag_offset.x;
    dragged_block->position.y = mouse_world.y - drag_offset.y;
  }

  if (dragging && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    dragging = false;
    dragged_block = nullptr;
  }
}

void draw_wire(const Vector2 &from, const Vector2 &to) {
  float dx = (to.x - from.x) * 0.5f;
  Vector2 cp1 = {from.x + dx, from.y};
  Vector2 cp2 = {to.x - dx, to.y};

  Vector2 prev = from;
  int segments = 30;
  for (int i = 1; i <= segments; i++) {
    float t = (float)i / (float)segments;
    float u = 1.0f - t;
    Vector2 pt;
    pt.x = u * u * u * from.x + 3 * u * u * t * cp1.x + 3 * u * t * t * cp2.x +
           t * t * t * to.x;
    pt.y = u * u * u * from.y + 3 * u * u * t * cp1.y + 3 * u * t * t * cp2.y +
           t * t * t * to.y;
    DrawLineEx(prev, pt, WIRE_THICK, COLOR_WIRE);
    prev = pt;
  }
}

void reset_input_state(InputFieldState &input_state) {
  input_state.active_node = nullptr;
  input_state.active_field = -1;
}

} // namespace ui
