#include "ui/graph.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <unordered_set>

Graph::Graph(const int shape)
    : dragged_block(nullptr), root(nullptr), leaf(nullptr),
      inference_ran(false), dragging(false), topology_dirty(false) {

  // TODO: We will only push an output and input block.

  blocks.push_back(std::make_unique<Block>(BlockType::PORT_INPUT,
                                           Vector2{200.0f, 200.0f}, shape));
  blocks.push_back(std::make_unique<Block>(BlockType::PORT_OUTPUT,
                                           Vector2{800.0f, 200.0f}, shape));
  blocks.push_back(
      std::make_unique<Block>(BlockType::RELU, Vector2{450.0f, 400.0f}, shape));

  Block *input_block = blocks[0].get();
  Block *output_block = blocks[1].get();
  Block *relu_block = blocks[2].get();

  this->root = input_block;
  this->leaf = output_block;

  connect(input_block, relu_block);
  connect(relu_block, output_block);

  InputFieldState input_state_val = InputFieldState{};
  this->input_state = std::make_unique<InputFieldState>(input_state_val);

  refresh_orphans();
}

void Graph::connect(Block *parent, Block *child) {
  for (auto *c : parent->next) {
    if (c == child)
      return;
  }
  parent->next.push_back(child);
  child->previous.push_back(parent);
  topology_dirty = true;
  refresh_orphans();
}

void Graph::disconnect(Block *parent, Block *child) {
  parent->next.erase(
      std::remove(parent->next.begin(), parent->next.end(), child),
      parent->next.end());
  child->previous.erase(
      std::remove(child->previous.begin(), child->previous.end(), parent),
      child->previous.end());
  topology_dirty = true;
  refresh_orphans();
}

bool Graph::is_reachable_from_root(Block *block) {
  if (root == nullptr)
    return false;
  std::unordered_set<Block *> visited;
  std::deque<Block *> queue = {root};
  visited.insert(root);
  while (!queue.empty()) {
    Block *cur = queue.front();
    queue.pop_front();
    if (cur == block)
      return true;
    for (auto *child : cur->next) {
      if (visited.insert(child).second) {
        queue.push_back(child);
      }
    }
  }
  return false;
}

void Graph::refresh_orphans() {
  orphans.clear();
  for (auto &bp : blocks) {
    Block *b = bp.get();
    if (b == root)
      continue;
    if (!is_reachable_from_root(b)) {
      orphans.push_back(b);
    }
  }
}

bool Graph::ready() { return true; }

void Graph::draw(const Camera2D &camera) {
  if (root) {
    std::unordered_set<Block *> visited;
    std::deque<Block *> queue = {root};
    visited.insert(root);

    while (!queue.empty()) {
      Block *current = queue.front();
      queue.pop_front();
      current->draw(*this->input_state);
      current->height = current->calculate_height();

      for (auto *child : current->next) {
        if (visited.insert(child).second) {
          queue.push_back(child);
          draw_wire(current->calculate_output_port(),
                    child->calculate_input_port());
        }
      }
    }
  }

  for (auto *orphan : orphans) {
    orphan->draw(*this->input_state);
    orphan->height = orphan->calculate_height();

    for (auto *child : orphan->next) {
      draw_wire(orphan->calculate_output_port(), child->calculate_input_port());
    }
  }

  if (connection_state.active && connection_state.from_block) {
    Vector2 from;
    if (connection_state.from_port == PortKind::OUTPUT) {
      from = connection_state.from_block->calculate_output_port();
    } else {
      from = connection_state.from_block->calculate_input_port();
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

Block *Graph::find_port_at(Vector2 cursor_pos, PortKind &out_port) {
  for (auto &bp : blocks) {
    Block *b = bp.get();

    if (b->type != BlockType::PORT_OUTPUT) {
      Vector2 op = b->calculate_output_port();
      float dx = cursor_pos.x - op.x;
      float dy = cursor_pos.y - op.y;
      if (dx * dx + dy * dy <= PORT_RADIUS * PORT_RADIUS * 4.0f) {
        out_port = PortKind::OUTPUT;
        return b;
      }
    }

    if (b->type != BlockType::PORT_INPUT) {
      Vector2 ip = b->calculate_input_port();
      float dx = cursor_pos.x - ip.x;
      float dy = cursor_pos.y - ip.y;
      if (dx * dx + dy * dy <= PORT_RADIUS * PORT_RADIUS * 4.0f) {
        out_port = PortKind::INPUT;
        return b;
      }
    }
  }
  return nullptr;
}

bool Graph::update(const Camera2D &camera) {
  bool inference_pressed = false;
  Vector2 mouse_screen = GetMousePosition();
  Vector2 mouse_world = GetScreenToWorld2D(mouse_screen, camera);

  Rectangle inference_rect = {(float)GetScreenWidth() - 160, 20, 140, 50};
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
      CheckCollisionPointRec(mouse_screen, inference_rect)) {
    inference_pressed = true;
  }

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !inference_pressed) {
    if (this->input_state->active_block != nullptr) {
      reset_input_state(*this->input_state);
    }

    PortKind clicked_port;
    Block *port_block = find_port_at(mouse_world, clicked_port);
    if (port_block) {
      connection_state.from_block = port_block;
      connection_state.from_port = clicked_port;
      connection_state.active = true;
      return inference_pressed;
    }

    bool clicked_field = false;
    Block *clicked = find_block_at(mouse_world);
    if (!clicked)
      return inference_pressed;
    Block &b = *clicked;

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
    PortKind clicked_port;
    Block *port_block = find_port_at(mouse_world, clicked_port);
    if (port_block) {
      if (clicked_port == PortKind::OUTPUT) {
        auto children_copy = port_block->next;
        for (auto *child : children_copy) {
          disconnect(port_block, child);
        }
      } else {
        auto parents_copy = port_block->previous;
        for (auto *parent : parents_copy) {
          disconnect(parent, port_block);
        }
      }
    }
  }

  if (connection_state.active && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    PortKind target_port;
    Block *target = find_port_at(mouse_world, target_port);

    if (target && target != connection_state.from_block) {
      if (connection_state.from_port == PortKind::OUTPUT &&
          target_port == PortKind::INPUT) {
        connect(connection_state.from_block, target);
      } else if (connection_state.from_port == PortKind::INPUT &&
                 target_port == PortKind::OUTPUT) {
        connect(target, connection_state.from_block);
      }
    }

    connection_state.active = false;
    connection_state.from_block = nullptr;
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

  // ── Block dragging ──────────────────────────────────────────────────────
  if (this->dragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    Block &b = *this->dragged_block;
    b.position.x = mouse_world.x - this->drag_offset.x;
    b.position.y = mouse_world.y - this->drag_offset.y;
  }

  if (this->dragging && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    this->dragging = false;
    this->dragged_block = nullptr;
  }

  return inference_pressed;
}

void draw_ui(const Graph &graph) {
  Rectangle inference_rect = {(float)GetScreenWidth() - 200, 20, 180, 50};
  bool hover = CheckCollisionPointRec(GetMousePosition(), inference_rect);

  DrawRectangleRounded(inference_rect, 0.3f, 6,
                       hover ? COLOR_INFERENCE_HOVER : COLOR_INFERENCE);
  DrawRectangleRoundedLinesEx(inference_rect, 0.3f, 6, 2.0f, BLACK);

  const char *inference_text = "INFERENCE";
  int tw = MeasureText(inference_text, 24);
  DrawText(inference_text,
           inference_rect.x + (inference_rect.width - tw) / 2.0f,
           inference_rect.y + 13, 24, WHITE);

  if (graph.inference_ran) {
    DrawText("Inference complete!", GetScreenWidth() - 200, 80, 18, DARKGREEN);
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
