#include "ui/graph.h"
#include "ui/config.h"
#include <cstdio>
#include <cstring>
#include <deque>
#include <memory>
#include <unordered_set>

Block::Block(const BlockType &type, const Vector2 &position, const int shape)
    : position(position) {
  for (int i = 0; i < shape; i++) {
    values.push_back(0.0f);
  }

  width = BLOCK_W;
  height = this->calculate_height();
  has_results = false;

  this->type = type;
  switch (this->type) {
    // TODO: In case we wan't to have special behaviour
  case BlockType::PORT_INPUT: {
    this->label = "INPUT";
    break;
  }
  case BlockType::PORT_OUTPUT: {
    this->label = "OUTPUT";
    break;
  }
  case BlockType::RELU: {
    this->label = "Relu";
    break;
  }
  }
}

float Block::calculate_height() {
  if (this->type == BlockType::PORT_INPUT) {
    return BLOCK_H_BASE + (FIELD_H + FIELD_PAD) * this->values.size() +
           FIELD_PAD;
  }
  if (this->type == BlockType::PORT_OUTPUT && this->has_results) {
    return BLOCK_H_BASE + (FIELD_H + FIELD_PAD) * this->values.size() +
           FIELD_PAD;
  }
  return BLOCK_H_BASE;
}

Vector2 Block::calculate_output_port() {
  float h = this->height;
  return {this->position.x + this->width, this->position.y + h / 2.0f};
}

Vector2 Block::calculate_input_port() {
  float h = this->height;
  return {this->position.x - 2.0f, this->position.y + h / 2.0f};
}

void Block::connect(std::unique_ptr<Block> child) {
  child->previous.push_back(this);
  this->next.push_back(std::move(child));
}

void Block::draw(const InputFieldState &input_state) {
  float h = this->height;
  Color block_color;

  switch (this->type) {
  case BlockType::PORT_INPUT:
    block_color = COLOR_PORT_INPUT;
    break;
  case BlockType::PORT_OUTPUT:
    block_color = COLOR_PORT_OUTPUT;
    break;
  case BlockType::RELU:
    block_color = COLOR_RELU;
    break;
  }

  DrawRectangleRounded({this->position.x, this->position.y, this->width, h},
                       0.15f, 6, block_color);
  DrawRectangleRoundedLinesEx(
      {this->position.x, this->position.y, this->width, h}, 0.15f, 6, 2.0f,
      BLACK);

  int font_size = 22;
  int text_w = MeasureText(this->label.c_str(), font_size);
  DrawText(this->label.c_str(),
           this->position.x + (this->width - text_w) / 2.0f,
           this->position.y + 8, font_size, BLACK);

  if (this->type != BlockType::PORT_INPUT) {
    Vector2 ip = this->calculate_input_port();
    DrawCircleV(ip, PORT_RADIUS, DARKGRAY);
    DrawCircleLines(ip.x, ip.y, PORT_RADIUS, BLACK);
  }
  Vector2 op = this->calculate_output_port();
  if (this->type != BlockType::PORT_OUTPUT) {
    DrawCircleV(op, PORT_RADIUS, DARKGRAY);
    DrawCircleLines(op.x, op.y, PORT_RADIUS, BLACK);
  }

  if (this->type == BlockType::PORT_INPUT) {
    float field_y = this->position.y + FIELD_START_H;
    for (size_t i = 0; i < this->values.size(); i++) {
      float fx = this->position.x + 10.0f;
      float fw = this->width - 20.0f;
      bool is_active = (input_state.active_block == this &&
                        input_state.active_field == (int)i);

      DrawRectangleRounded({fx, field_y, fw, FIELD_H}, 0.3f, 4,
                           is_active ? COLOR_FIELD_ACTIVE : COLOR_FIELD_BG);
      DrawRectangleRoundedLinesEx({fx, field_y, fw, FIELD_H}, 0.3f, 4, 1.0f,
                                  is_active ? RED : DARKGRAY);

      if (is_active) {
        DrawText(input_state.buffer, fx + 6, field_y + 5, 18, BLACK);
        if (((int)(GetTime() * 2)) % 2 == 0) {
          int cursor_x = fx + 6 + MeasureText(input_state.buffer, 18);
          DrawLine(cursor_x, field_y + 4, cursor_x, field_y + FIELD_H - 4, RED);
        }
      } else {
        char val_text[FLOAT_BUFFER_SIZE];
        snprintf(val_text, sizeof(val_text), "%.2f", this->values[i]);
        DrawText(val_text, fx + 6, field_y + 5, 18, DARKGRAY);
      }

      field_y += FIELD_H + FIELD_PAD;
    }
  }

  if (this->type == BlockType::PORT_OUTPUT && this->has_results) {
    float field_y = this->position.y + FIELD_START_H;
    for (size_t i = 0; i < this->values.size(); i++) {
      float fx = this->position.x + 10.0f;
      float fw = this->width - 20.0f;

      DrawRectangleRounded({fx, field_y, fw, FIELD_H}, 0.3f, 4,
                           COLOR_RESULT_BG);
      DrawRectangleRoundedLinesEx({fx, field_y, fw, FIELD_H}, 0.3f, 4, 1.0f,
                                  DARKGRAY);

      char val_text[FLOAT_BUFFER_SIZE];
      snprintf(val_text, sizeof(val_text), "%.4f", this->values[i]);
      DrawText(val_text, fx + 6, field_y + 5, 18, BLACK);

      field_y += FIELD_H + FIELD_PAD;
    }
  }
}

Graph::Graph(const int shape)
    : dragged_block(nullptr), inference_ran(false), dragging(false) {
  this->root = std::make_unique<Block>(BlockType::PORT_INPUT,
                                       Vector2{200.0f, 200.0f}, shape);
  auto output = std::make_unique<Block>(BlockType::PORT_OUTPUT,
                                        Vector2{800.0f, 200.0f}, shape);
  this->leaf = output.get();
  this->root->connect(std::move(output));

  InputFieldState input_state = InputFieldState{};
  this->input_state = std::make_unique<InputFieldState>(input_state);
};

bool Graph::ready() { return true; }

void Graph::draw() {
  std::unordered_set<Block *> visited;
  std::deque<Block *> queue = {this->root.get()};
  visited.insert(this->root.get());

  while (!queue.empty()) {
    Block *current = queue.front();
    queue.pop_front();
    current->draw(*this->input_state);

    for (auto &child : current->next) {
      if (visited.insert(child.get()).second) {
        queue.push_back(child.get());
        draw_wire(current->calculate_output_port(),
                  child->calculate_input_port());
      }
    }
  }
}

Block *Graph::find_block_at(Vector2 cursor_pos) {
  std::unordered_set<Block *> visited;
  std::deque<Block *> queue = {this->root.get()};
  visited.insert(this->root.get());

  while (!queue.empty()) {
    Block *current = queue.front();
    queue.pop_front();

    float h = current->height;
    Rectangle block_rect = {current->position.x, current->position.y,
                            current->width, h};
    if (CheckCollisionPointRec(cursor_pos, block_rect)) {
      return current;
    }

    for (auto &child : current->next) {
      if (visited.insert(child.get()).second) {
        queue.push_back(child.get());
      }
    }
  }
  return nullptr;
}

bool Graph::update(const Camera2D &camera) {
  bool inference_pressed = false;
  Vector2 mouse_screen = GetMousePosition();
  Vector2 mouse_world = GetScreenToWorld2D(mouse_screen, camera);

  Rectangle inference_rect = {(float)GetScreenWidth() - 160, 20, 140,
                              50}; // TODO: Make this a button reference
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
      CheckCollisionPointRec(mouse_screen, inference_rect)) {
    inference_pressed = true;
  }

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !inference_pressed) {
    if (this->input_state->active_block != nullptr) {
      reset_input_state(*this->input_state);
    }
    bool clicked_field = false;
    Block *clicked = find_block_at(mouse_world);
    if (!clicked)
      return inference_pressed;
    Block &b = *clicked;

    float field_y = b.position.y + FIELD_START_H; // TODO: Fix magic number
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

// void run_graph_inference(GraphState &state) {
//   if (state.blocks.size() < 3)
//     return;
//
//   Block &input_block = *state.blocks[0];
//
//   Block &output_block = *state.blocks[2];
//   output_block.values.resize(input_block.values.size());
//   for (size_t i = 0; i < input_block.values.size(); i++) {
//     output_block.values[i] = std::max(0.0f, input_block.values[i]);
//   }
//   output_block.has_results = true;
//   state.inference_ran = true;
// }
