#include "ui/graph.h"
#include "raylib.h"
#include "ui/block.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <unordered_set>

Graph::Graph(const size_t shape)
    : dragged_block(nullptr), inference_ran(false), dragging(false),
      topology_dirty(false) {
  // TODO: We will only push an output and input block.

  blocks.push_back(std::make_unique<Block>("PortInput",
                                           generate_block_label("PortInput"),
                                           Vector2{200.0f, 200.0f}, shape));
  blocks.push_back(std::make_unique<Block>("PortOutput",
                                           generate_block_label("PortOutput"),
                                           Vector2{800.0f, 200.0f}, shape));
  blocks.push_back(std::make_unique<Block>("Relu", generate_block_label("Relu"),
                                           Vector2{450.0f, 400.0f}, shape));

  Block *input_block = blocks[0].get();
  Block *output_block = blocks[1].get();
  Block *relu_block = blocks[2].get();

  this->roots = {input_block};
  this->leafs = {output_block};

  connect(input_block, relu_block, 0, 0);
  connect(relu_block, output_block, 0, 0);

  InputFieldState input_state_val = InputFieldState{};

  this->input_state = std::make_unique<InputFieldState>(input_state_val);
  refresh_orphans();
}

std::string Graph::generate_block_label(const std::string op) {
  int count = 0;
  std::string label = op;

  while (block_label_exists(label)) {
    count++;
    label = op + std::to_string(count);
  }

  return label;
}

int Graph::count_blocks_with_type(const std::string type) {
  int count = 0;
  for (const auto &bp : blocks) {
    if (bp->definition->name == type) {
      count++;
    }
  }
  for (const auto &orphan : orphans) {
    if (orphan->definition->name == type) {
      count++;
    }
  }
  return count;
}

bool Graph::block_label_exists(const std::string &label) {
  for (const auto &bp : blocks) {
    if (bp->label == label) {
      return true;
    }
  }
  for (const auto &orphan : orphans) {
    if (orphan->label == label) {
      return true;
    }
  }
  return false;
}

void Graph::push_block(Block *block) {
  blocks.push_back(std::unique_ptr<Block>(block));
  if (block->definition->name == "PortInput") {
    roots.push_back(block);
  }

  if (block->definition->name == "PortOutput") {
    leafs.push_back(block);
  }

  topology_dirty = true;
  refresh_orphans();
}

void Graph::remove_block(Block *block) {
  for (auto input : block->inputs) {
    auto &connections = input.block->outputs;
    connections.erase(std::remove_if(connections.begin(), connections.end(),
                                     [block](const Connection &c) {
                                       return c.block == block;
                                     }),
                      connections.end());
  }
  for (auto output : block->outputs) {
    auto &connections = output.block->inputs;
    connections.erase(std::remove_if(connections.begin(), connections.end(),
                                     [block](const Connection &c) {
                                       return c.block == block;
                                     }),
                      connections.end());
  }
  auto it = std::find_if(blocks.begin(), blocks.end(),
                         [block](const auto &bp) { return bp.get() == block; });
  if (it->get()->definition->name == "PortInput") {
    roots.erase(std::remove(roots.begin(), roots.end(), block), roots.end());
  }
  if (it->get()->definition->name == "PortOutput") {
    leafs.erase(std::remove(leafs.begin(), leafs.end(), block), leafs.end());
  }
  if (it != blocks.end()) {
    blocks.erase(it);
  }
  topology_dirty = true;
  refresh_orphans();
}

void Graph::connect(Block *parent, Block *child, const size_t out_port_index,
                    const size_t in_port_index) {
  for (auto c : parent->inputs) {
    if (c.block == child)
      return;
  }
  parent->outputs.push_back({child, out_port_index});
  child->inputs.push_back({parent, in_port_index});
  topology_dirty = true;
  refresh_orphans();
}

void Graph::disconnect(Block *parent, Block *child, const size_t out_port_index,
                       const size_t in_port_index) {
  parent->outputs.erase(
      std::remove_if(parent->outputs.begin(), parent->outputs.end(),
                     [child, out_port_index](const Connection &c) {
                       return c.block == child &&
                              c.port_index == out_port_index;
                     }),
      parent->outputs.end());
  child->inputs.erase(
      std::remove_if(child->inputs.begin(), child->inputs.end(),
                     [parent, in_port_index](const Connection &c) {
                       return c.block == parent &&
                              c.port_index == in_port_index;
                     }),
      child->inputs.end());
  topology_dirty = true;
  refresh_orphans();
}

bool Graph::is_reachable_from_root(Block *block) {
  if (roots.empty())
    return false;

  std::deque<Block *> queue(roots.begin(), roots.end());
  std::unordered_set<Block *> visited(roots.begin(), roots.end());

  while (!queue.empty()) {
    Block *cur = queue.front();
    queue.pop_front();
    if (cur == block)
      return true;
    for (auto child : cur->outputs) {
      if (visited.insert(child.block).second) {
        queue.push_back(child.block);
      }
    }
  }
  return false;
}

void Graph::refresh_orphans() {
  orphans.clear();
  for (auto &bp : blocks) {
    Block *b = bp.get();
    if (std::find(roots.begin(), roots.end(), b) != roots.end())
      continue;
    if (!is_reachable_from_root(b)) {
      orphans.push_back(b);
    }
  }
}

bool Graph::ready() { return true; }

bool Graph::popup_active() const { return shape_popup.active; }

void Graph::open_shape_popup(Block *b) {
  shape_popup.target = b;
  shape_popup.pending_shape = (int)b->values.size();
  shape_popup.active = true;
}

void Graph::draw_popup() {
  if (!shape_popup.active)
    return;

  int sw = GetScreenWidth();
  int sh = GetScreenHeight();

  DrawRectangle(0, 0, sw, sh, {0, 0, 0, 120});

  const float pw = 300.0f, ph = 200.0f;
  float px = (sw - pw) / 2.0f;
  float py = (sh - ph) / 2.0f;

  DrawRectangleRec({px, py, pw, ph}, {35, 35, 40, 255});
  DrawRectangleLinesEx({px, py, pw, ph}, 1.5f, {70, 70, 75, 255});

  DrawRectangleRec({px, py, pw, 40.0f}, {25, 25, 30, 255});
  DrawText("Configure Input Shape", px + 12, py + 12, 15, WHITE);

  std::string block_lbl = "Block: " + shape_popup.target->label;
  DrawText(block_lbl.c_str(), px + 12, py + 52, 14, LIGHTGRAY);
  DrawText("Number of input values:", px + 12, py + 74, 14, LIGHTGRAY);

  const float btn_w = 34.0f, btn_h = 34.0f, cnt_w = 54.0f;
  float controls_w = btn_w + cnt_w + btn_w + 8.0f;
  float cx = px + (pw - controls_w) / 2.0f;
  float cy = py + 104.0f;

  Rectangle minus_r = {cx, cy, btn_w, btn_h};
  Rectangle count_r = {cx + btn_w + 4, cy, cnt_w, btn_h};
  Rectangle plus_r = {cx + btn_w + cnt_w + 8, cy, btn_w, btn_h};

  Vector2 mouse = GetMousePosition();
  bool minus_hov = CheckCollisionPointRec(mouse, minus_r);
  bool plus_hov = CheckCollisionPointRec(mouse, plus_r);

  DrawRectangleRec(minus_r, minus_hov ? Color{100, 100, 110, 255}
                                      : Color{65, 65, 75, 255});
  DrawRectangleLinesEx(minus_r, 1.0f, {90, 90, 100, 255});
  DrawText("-", minus_r.x + (btn_w - MeasureText("-", 22)) / 2, cy + 5, 22,
           WHITE);

  DrawRectangleRec(count_r, {55, 55, 62, 255});
  DrawRectangleLinesEx(count_r, 1.0f, {80, 80, 90, 255});
  char cs[8];
  snprintf(cs, sizeof(cs), "%d", shape_popup.pending_shape);
  DrawText(cs, count_r.x + (cnt_w - MeasureText(cs, 20)) / 2, cy + 7, 20,
           WHITE);

  DrawRectangleRec(plus_r, plus_hov ? Color{100, 100, 110, 255}
                                    : Color{65, 65, 75, 255});
  DrawRectangleLinesEx(plus_r, 1.0f, {90, 90, 100, 255});
  DrawText("+", plus_r.x + (btn_w - MeasureText("+", 22)) / 2, cy + 5, 22,
           WHITE);

  float bw = 90.0f, bh = 32.0f;
  float by = py + ph - 46.0f;
  Rectangle cancel_r = {px + 24, by, bw, bh};
  Rectangle ok_r = {px + pw - 24 - bw, by, bw, bh};

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
    if (minus_hov && shape_popup.pending_shape > 1)
      shape_popup.pending_shape--;
    else if (plus_hov && shape_popup.pending_shape < 64)
      shape_popup.pending_shape++;
    else if (ok_hov) {
      shape_popup.target->values.resize(shape_popup.pending_shape, 0.0f);
      reset_input_state(*input_state);
      shape_popup.active = false;
      shape_popup.target = nullptr;
    } else if (cancel_hov) {
      shape_popup.active = false;
      shape_popup.target = nullptr;
    }
  }

  if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
    shape_popup.target->values.resize(shape_popup.pending_shape, 0.0f);
    reset_input_state(*input_state);
    shape_popup.active = false;
    shape_popup.target = nullptr;
  }

  if (IsKeyPressed(KEY_ESCAPE)) {
    shape_popup.active = false;
    shape_popup.target = nullptr;
  }
}

void Graph::draw_grid(const Camera2D &camera) {
  float gridSpacing = 150.0f;
  float invZoom = 1.0f / camera.zoom;
  float halfW = GetScreenWidth() * invZoom;
  float halfH = GetScreenHeight() * invZoom;

  float startX = floorf((camera.target.x - halfW) / gridSpacing) * gridSpacing;
  float startY = floorf((camera.target.y - halfH) / gridSpacing) * gridSpacing;
  float endX = camera.target.x + halfW;
  float endY = camera.target.y + halfH;

  for (float x = startX; x <= endX; x += gridSpacing) {
    DrawLineV({x, startY}, {x, endY}, LIGHTGRAY);
  }
  for (float y = startY; y <= endY; y += gridSpacing) {
    DrawLineV({startX, y}, {endX, y}, LIGHTGRAY);
  }
}

void Graph::draw(const Camera2D &camera) {
  draw_grid(camera);

  if (!roots.empty()) {
    std::deque<Block *> queue(roots.begin(), roots.end());
    std::unordered_set<Block *> visited(roots.begin(), roots.end());

    while (!queue.empty()) {
      Block *curr = queue.front();
      queue.pop_front();
      curr->draw(*this->input_state);
      curr->height = curr->calculate_height();

      for (auto child : curr->outputs) {
        if (visited.insert(child.block).second) {
          queue.push_back(child.block);
        }
        size_t in_port_index =
            std::find_if(
                child.block->inputs.begin(), child.block->inputs.end(),
                [curr](const Connection &c) { return c.block == curr; })
                ->port_index;

        draw_wire(curr->calculate_output_ports()[child.port_index],
                  child.block->calculate_input_ports()[in_port_index]);
      }
    }
  }

  for (auto *orphan : orphans) {
    orphan->draw(*this->input_state);
    orphan->height = orphan->calculate_height();

    for (auto child : orphan->outputs) {
      size_t in_port_index =
          std::find_if(
              child.block->inputs.begin(), child.block->inputs.end(),
              [orphan](const Connection &c) { return c.block == orphan; })
              ->port_index;
      draw_wire(orphan->calculate_output_ports()[child.port_index],
                child.block->calculate_input_ports()[in_port_index]);
    }
  }

  if (connection_state.active && connection_state.connection.block) {
    Vector2 from;
    if (connection_state.from_port == PortKind::OUTPUT) {
      from = connection_state.connection.block->calculate_output_ports()[0];
    } else {
      from = connection_state.connection.block->calculate_input_ports()[0];
    }
    Vector2 mouse_world = GetScreenToWorld2D(GetMousePosition(), camera);
    draw_wire(from, mouse_world);
  }
}

Block *Graph::find_block_at(Vector2 cursor_pos) {
  for (auto &bp : blocks) {
    Block *b = bp.get();
    float h = b->height;
    Rectangle block_rect = {b->position.x, b->position.y, b->width, h};
    if (CheckCollisionPointRec(cursor_pos, block_rect)) {
      return b;
    }
  }
  return nullptr;
}

Connection Graph::find_port_at(Vector2 cursor_pos, PortKind &out_port) {
  for (auto &bp : blocks) {
    Block *b = bp.get();
    auto definition = b->definition;

    if (definition->num_outputs > 0) {
      auto out_ports = b->calculate_output_ports();
      for (size_t i = 0; i < out_ports.size(); i++) {
        float dx = cursor_pos.x - out_ports[i].x;
        float dy = cursor_pos.y - out_ports[i].y;
        if (dx * dx + dy * dy <= PORT_RADIUS * PORT_RADIUS) {
          out_port = PortKind::OUTPUT;
          return {b, i};
        }
      }
    }

    if (definition->num_inputs > 0) {
      auto in_ports = b->calculate_input_ports();
      for (size_t i = 0; i < in_ports.size(); i++) {
        float dx = cursor_pos.x - in_ports[i].x;
        float dy = cursor_pos.y - in_ports[i].y;
        if (dx * dx + dy * dy <= PORT_RADIUS * PORT_RADIUS) {
          out_port = PortKind::INPUT;
          return {b, i};
        }
      }
    }
  }

  return {nullptr, 99};
}

void Graph::update(const Camera2D &camera) {
  if (shape_popup.active)
    return;

  Vector2 mouse_screen = GetMousePosition();
  Vector2 mouse_world = GetScreenToWorld2D(mouse_screen, camera);

  PortKind kind;
  auto *hovered_block = find_block_at(mouse_world);
  auto hovered_port = find_port_at(mouse_world, kind);
  if (hovered_block != NULL) {
    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
  } else if (hovered_port.block == NULL) {
    SetMouseCursor(MOUSE_CURSOR_CROSSHAIR);
  } else {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
  }

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
    if (this->input_state->active_block != nullptr) {
      reset_input_state(*this->input_state);
    }

    PortKind clicked_port;
    auto connection = find_port_at(mouse_world, clicked_port);
    if (connection.block) {
      connection_state.connection.block = connection.block;
      connection_state.connection.port_index = connection.port_index;
      connection_state.from_port = clicked_port;
      connection_state.active = true;
      return;
    }

    bool clicked_field = false;
    Block *clicked = find_block_at(mouse_world);
    if (!clicked) {
      return;
    }
    Block &b = *clicked;

    if (b.definition->name == "PortInput") {
      Rectangle header_rect = {b.position.x, b.position.y, b.width,
                               FIELD_START_H};
      if (CheckCollisionPointRec(mouse_world, header_rect)) {
        double now = GetTime();
        if (now - last_click_time < 0.35 && last_click_block == &b) {
          open_shape_popup(&b);
          last_click_block = nullptr;
          return;
        }
        last_click_time = now;
        last_click_block = &b;
      }
    }

    float field_y = b.position.y + FIELD_START_H;
    for (size_t fi = 0; fi < b.values.size(); fi++) {
      Rectangle field_rect = {b.position.x + 10.0f, field_y, b.width - 20.0f,
                              FIELD_H};
      if (CheckCollisionPointRec(mouse_world, field_rect)) {
        if (this->input_state->active_block != nullptr &&
            this->input_state->active_field >= 0) {
          Block &prev = *this->input_state->active_block;
          float val = (float)atof(this->input_state->buffer);
          prev.values[this->input_state->active_field] = val;
        }

        this->input_state->active_block = &b;
        this->input_state->active_field = (int)fi;
        snprintf(this->input_state->buffer, sizeof(this->input_state->buffer),
                 "%.2f", b.values[fi]);
        this->input_state->cursor = (int)strlen(this->input_state->buffer);
        clicked_field = true;
        break;
      }

      field_y += FIELD_H + FIELD_PAD;
    }

    this->dragging = true;
    this->dragged_block = &b;
    this->drag_offset = {mouse_world.x - b.position.x,
                         mouse_world.y - b.position.y};

    if (!clicked_field && this->input_state->active_block != nullptr) {
      Block &prev = *this->input_state->active_block;
      float val = (float)atof(this->input_state->buffer);
      prev.values[this->input_state->active_field] = val;
      reset_input_state(*this->input_state);
    }
  }

  if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
    Block *clicked = find_block_at(mouse_world);
    if (clicked != NULL) {
      remove_block(clicked);
    }

    PortKind clicked_port;
    Connection target = find_port_at(mouse_world, clicked_port);
    if (target.block) {
      if (clicked_port == PortKind::OUTPUT) {
        auto outputs = target.block->outputs;
        for (auto output : outputs) {
          disconnect(target.block, output.block, output.port_index,
                     target.port_index);
        }
      } else {
        auto inputs = target.block->inputs;
        for (auto input : inputs) {
          disconnect(input.block, target.block, input.port_index,
                     target.port_index);
        }
      }
    }
  }

  if (connection_state.active && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    PortKind target_port;
    Connection target = find_port_at(mouse_world, target_port);

    printf("target: %s, target_port: %zu\n",
           target.block ? target.block->label.c_str() : "null",
           target.port_index);

    if (target.block && target.block != connection_state.connection.block) {
      if (connection_state.from_port == PortKind::OUTPUT &&
          target_port == PortKind::INPUT) {
        connect(connection_state.connection.block, target.block,
                connection_state.connection.port_index, target.port_index);
      } else if (connection_state.from_port == PortKind::INPUT &&
                 target_port == PortKind::OUTPUT) {
        connect(target.block, connection_state.connection.block,
                target.port_index, connection_state.connection.port_index);
      }
    }

    connection_state.active = false;
    connection_state.connection.block = nullptr;
  }

  if (this->input_state->active_block != nullptr) {
    int key = GetCharPressed();
    while (key > 0) {
      if ((key >= '0' && key <= '9') || key == '-' || key == '.') {
        int len = (int)strlen(this->input_state->buffer);
        if (len < 30) {
          this->input_state->buffer[len] = (char)key;
          this->input_state->buffer[len + 1] = '\0';
          this->input_state->cursor++;
        }
      }
      key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
      int len = (int)strlen(this->input_state->buffer);
      if (len > 0) {
        this->input_state->buffer[len - 1] = '\0';
        this->input_state->cursor--;
      }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
      Block &b = *this->input_state->active_block;
      float val = (float)atof(this->input_state->buffer);
      b.values[this->input_state->active_field] = val;
      reset_input_state(*this->input_state);
    }

    if (IsKeyPressed(KEY_TAB) && IsKeyDown(KEY_LEFT_SHIFT)) {
      Block &b = *this->input_state->active_block;
      float val = (float)atof(this->input_state->buffer);
      b.values[this->input_state->active_field] = val;

      int previous_field = this->input_state->active_field - 1;
      if (previous_field >= 0) {
        this->input_state->active_field = previous_field;
        snprintf(this->input_state->buffer, sizeof(this->input_state->buffer),
                 "%.2f", b.values[previous_field]);
        this->input_state->cursor = (int)strlen(this->input_state->buffer);
      } else {
        reset_input_state(*this->input_state);
      }

    } else if (IsKeyPressed(KEY_TAB)) {
      Block &b = *this->input_state->active_block;
      float val = (float)atof(this->input_state->buffer);
      b.values[this->input_state->active_field] = val;

      int next_field = this->input_state->active_field + 1;
      if (next_field < (int)b.values.size()) {
        this->input_state->active_field = next_field;
        snprintf(this->input_state->buffer, sizeof(this->input_state->buffer),
                 "%.2f", b.values[next_field]);
        this->input_state->cursor = (int)strlen(this->input_state->buffer);
      } else {
        reset_input_state(*this->input_state);
      }
    }
  }

  if (this->dragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    Block &b = *this->dragged_block;
    b.position.x = mouse_world.x - this->drag_offset.x;
    b.position.y = mouse_world.y - this->drag_offset.y;
  }

  if (this->dragging && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    this->dragging = false;
    this->dragged_block = nullptr;
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
  input_state.active_block = nullptr;
  input_state.active_field = -1;
}
