#include "ui/graph.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <iostream>

static constexpr float BLOCK_W = 180.0f;
static constexpr float BLOCK_H_BASE = 60.0f;
static constexpr float FIELD_H = 28.0f;
static constexpr float FIELD_PAD = 4.0f;
static constexpr float PORT_RADIUS = 8.0f;
static constexpr float WIRE_THICK = 3.0f;

static const Color COLOR_PORT_INPUT = {100, 180, 255, 255};  // Blue
static const Color COLOR_PORT_OUTPUT = {100, 220, 130, 255}; // Green
static const Color COLOR_OPERATOR = {255, 180, 80, 255};     // Orange
static const Color COLOR_FIELD_BG = {240, 240, 240, 255};
static const Color COLOR_FIELD_ACTIVE = {255, 255, 220, 255};
static const Color COLOR_WIRE = {80, 80, 80, 255};
static const Color COLOR_INFERENCE = {80, 200, 80, 255};
static const Color COLOR_INFERENCE_HOVER = {60, 240, 60, 255};
static const Color COLOR_RESULT_BG = {220, 245, 220, 255};

static float block_height(const Block &b) {
  if (b.type == BlockType::PORT_INPUT) {
    return BLOCK_H_BASE + (FIELD_H + FIELD_PAD) * b.values.size() + FIELD_PAD;
  }
  if (b.type == BlockType::PORT_OUTPUT && b.has_results) {
    return BLOCK_H_BASE + (FIELD_H + FIELD_PAD) * b.values.size() + FIELD_PAD;
  }
  return BLOCK_H_BASE;
}

static Vector2 block_output_port(const Block &b) {
  float h = block_height(b);
  return {b.position.x + b.width, b.position.y + h / 2.0f};
}

static Vector2 block_input_port(const Block &b) {
  float h = block_height(b);
  return {b.position.x, b.position.y + h / 2.0f};
}

GraphState create_default_graph() {
  GraphState state = {};
  state.input_state = {-1, -1, {0}, 0};
  state.inference_ran = false;
  state.dragging = false;
  state.dragged_block = -1;

  Block x_block;
  x_block.type = BlockType::PORT_INPUT;
  x_block.label = "X";
  x_block.position = {200.0f, 200.0f};
  x_block.width = BLOCK_W;
  x_block.height = BLOCK_H_BASE;
  x_block.values = {0.0f, 0.0f, 0.0f, 0.0f};
  x_block.has_results = false;
  state.blocks.push_back(x_block);

  Block relu_block;
  relu_block.type = BlockType::OPERATOR;
  relu_block.label = "RELU";
  relu_block.position = {500.0f, 250.0f};
  relu_block.width = BLOCK_W;
  relu_block.height = BLOCK_H_BASE;
  relu_block.has_results = false;
  state.blocks.push_back(relu_block);

  Block y_block;
  y_block.type = BlockType::PORT_OUTPUT;
  y_block.label = "Y";
  y_block.position = {800.0f, 200.0f};
  y_block.width = BLOCK_W;
  y_block.height = BLOCK_H_BASE;
  y_block.values = {};
  y_block.has_results = false;
  state.blocks.push_back(y_block);

  return state;
}

static void draw_block(const Block &b, const InputFieldState &input_state,
                       int block_index) {
  float h = block_height(b);
  Color block_color;

  switch (b.type) {
  case BlockType::PORT_INPUT:
    block_color = COLOR_PORT_INPUT;
    break;
  case BlockType::PORT_OUTPUT:
    block_color = COLOR_PORT_OUTPUT;
    break;
  case BlockType::OPERATOR:
    block_color = COLOR_OPERATOR;
    break;
  }

  DrawRectangleRounded({b.position.x, b.position.y, b.width, h}, 0.15f, 6,
                       block_color);
  DrawRectangleRoundedLinesEx({b.position.x, b.position.y, b.width, h}, 0.15f,
                              6, 2.0f, BLACK);

  int font_size = 22;
  int text_w = MeasureText(b.label.c_str(), font_size);
  DrawText(b.label.c_str(), b.position.x + (b.width - text_w) / 2.0f,
           b.position.y + 8, font_size, BLACK);

  if (b.type != BlockType::PORT_INPUT) {
    Vector2 ip = block_input_port(b);
    DrawCircleV(ip, PORT_RADIUS, DARKGRAY);
    DrawCircleLines(ip.x, ip.y, PORT_RADIUS, BLACK);
  }
  if (b.type != BlockType::PORT_OUTPUT) {
    Vector2 op = block_output_port(b);
    DrawCircleV(op, PORT_RADIUS, DARKGRAY);
    DrawCircleLines(op.x, op.y, PORT_RADIUS, BLACK);
  }

  if (b.type == BlockType::PORT_INPUT) {
    float field_y = b.position.y + 38.0f;
    for (size_t i = 0; i < b.values.size(); i++) {
      float fx = b.position.x + 10.0f;
      float fw = b.width - 20.0f;
      bool is_active = (input_state.active_block == block_index &&
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
        char val_text[32];
        snprintf(val_text, sizeof(val_text), "%.2f", b.values[i]);
        DrawText(val_text, fx + 6, field_y + 5, 18, DARKGRAY);
      }

      field_y += FIELD_H + FIELD_PAD;
    }
  }

  if (b.type == BlockType::PORT_OUTPUT && b.has_results) {
    float field_y = b.position.y + 38.0f;
    for (size_t i = 0; i < b.values.size(); i++) {
      float fx = b.position.x + 10.0f;
      float fw = b.width - 20.0f;

      DrawRectangleRounded({fx, field_y, fw, FIELD_H}, 0.3f, 4,
                           COLOR_RESULT_BG);
      DrawRectangleRoundedLinesEx({fx, field_y, fw, FIELD_H}, 0.3f, 4, 1.0f,
                                  DARKGRAY);

      char val_text[32];
      snprintf(val_text, sizeof(val_text), "%.4f", b.values[i]);
      DrawText(val_text, fx + 6, field_y + 5, 18, BLACK);

      field_y += FIELD_H + FIELD_PAD;
    }
  }
}

static void draw_wire(Vector2 from, Vector2 to) {
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

void draw_graph(const GraphState &state) {
  if (state.blocks.size() >= 3) {
    Vector2 x_out = block_output_port(state.blocks[0]);
    Vector2 relu_in = block_input_port(state.blocks[1]);
    draw_wire(x_out, relu_in);

    Vector2 relu_out = block_output_port(state.blocks[1]);
    Vector2 y_in = block_input_port(state.blocks[2]);
    draw_wire(relu_out, y_in);
  }

  for (size_t i = 0; i < state.blocks.size(); i++) {
    draw_block(state.blocks[i], state.input_state, (int)i);
  }
}

void draw_graph_ui(const GraphState &state) {
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

  if (state.inference_ran) {
    DrawText("Inference complete!", GetScreenWidth() - 220, 80, 18, DARKGREEN);
  }
}

bool update_graph(GraphState &state, const Camera2D &camera) {
  bool play_pressed = false;
  Vector2 mouse_screen = GetMousePosition();
  Vector2 mouse_world = GetScreenToWorld2D(mouse_screen, camera);

  Rectangle play_rect = {(float)GetScreenWidth() - 160, 20, 140, 50};
  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
      CheckCollisionPointRec(mouse_screen, play_rect)) {
    play_pressed = true;
  }

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !play_pressed) {
    bool clicked_field = false;

    for (size_t bi = 0; bi < state.blocks.size(); bi++) {
      Block &b = state.blocks[bi];
      if (b.type != BlockType::PORT_INPUT)
        continue;

      float field_y = b.position.y + 38.0f;
      for (size_t fi = 0; fi < b.values.size(); fi++) {
        Rectangle field_rect = {b.position.x + 10.0f, field_y, b.width - 20.0f,
                                FIELD_H};
        if (CheckCollisionPointRec(mouse_world, field_rect)) {
          if (state.input_state.active_block >= 0 &&
              state.input_state.active_field >= 0) {
            Block &prev = state.blocks[state.input_state.active_block];
            float val = (float)atof(state.input_state.buffer);
            prev.values[state.input_state.active_field] = val;
          }

          state.input_state.active_block = (int)bi;
          state.input_state.active_field = (int)fi;
          snprintf(state.input_state.buffer, sizeof(state.input_state.buffer),
                   "%.2f", b.values[fi]);
          state.input_state.cursor = (int)strlen(state.input_state.buffer);
          clicked_field = true;
          break;
        }
        field_y += FIELD_H + FIELD_PAD;
      }
      if (clicked_field)
        break;
    }

    if (!clicked_field && state.input_state.active_block >= 0) {
      Block &prev = state.blocks[state.input_state.active_block];
      float val = (float)atof(state.input_state.buffer);
      prev.values[state.input_state.active_field] = val;
      state.input_state.active_block = -1;
      state.input_state.active_field = -1;
    }
  }

  if (state.input_state.active_block >= 0) {
    int key = GetCharPressed();
    while (key > 0) {
      if ((key >= '0' && key <= '9') || key == '-' || key == '.') {
        int len = (int)strlen(state.input_state.buffer);
        if (len < 30) {
          state.input_state.buffer[len] = (char)key;
          state.input_state.buffer[len + 1] = '\0';
          state.input_state.cursor++;
        }
      }
      key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
      int len = (int)strlen(state.input_state.buffer);
      if (len > 0) {
        state.input_state.buffer[len - 1] = '\0';
        state.input_state.cursor--;
      }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
      Block &b = state.blocks[state.input_state.active_block];
      float val = (float)atof(state.input_state.buffer);
      b.values[state.input_state.active_field] =
          val; // TODO: Refactor this to a function, update_field
      state.input_state.active_block = -1;
      state.input_state.active_field = -1;
    }

    if (IsKeyPressed(KEY_TAB) && IsKeyDown(KEY_LEFT_SHIFT)) {
      Block &b = state.blocks[state.input_state.active_block];
      float val = (float)atof(state.input_state.buffer);
      b.values[state.input_state.active_field] = val;

      int previous_field = state.input_state.active_field - 1;
      if (previous_field >= 0) {
        state.input_state.active_field = previous_field;
        snprintf(state.input_state.buffer, sizeof(state.input_state.buffer),
                 "%.2f", b.values[previous_field]); // TODO: Refactor this to a
                                                    // format function
        state.input_state.cursor = (int)strlen(state.input_state.buffer);
      } else {
        state.input_state.active_block = -1;
        state.input_state.active_field = -1;
      }

    } else if (IsKeyPressed(KEY_TAB)) {
      Block &b = state.blocks[state.input_state.active_block];
      float val = (float)atof(state.input_state.buffer);
      b.values[state.input_state.active_field] = val;

      int next_field = state.input_state.active_field + 1;
      if (next_field < (int)b.values.size()) {
        state.input_state.active_field = next_field;
        snprintf(state.input_state.buffer, sizeof(state.input_state.buffer),
                 "%.2f", b.values[next_field]);
        state.input_state.cursor = (int)strlen(state.input_state.buffer);
      } else {
        state.input_state.active_block = -1;
        state.input_state.active_field = -1;
      }
    }
  }

  if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
      state.input_state.active_block < 0 && !play_pressed) {
    for (size_t i = 0; i < state.blocks.size(); i++) {
      Block &b = state.blocks[i];
      float h = block_height(b);
      Rectangle block_rect = {b.position.x, b.position.y, b.width, h};
      if (CheckCollisionPointRec(mouse_world, block_rect)) {
        if (b.type == BlockType::PORT_INPUT) {
          float field_y = b.position.y + 38.0f;
          bool in_field = false;
          for (size_t fi = 0; fi < b.values.size(); fi++) {
            Rectangle fr = {b.position.x + 10.0f, field_y, b.width - 20.0f,
                            FIELD_H};
            if (CheckCollisionPointRec(mouse_world, fr)) {
              in_field = true;
              break;
            }
            field_y += FIELD_H + FIELD_PAD;
          }
          if (in_field)
            continue;
        }
        state.dragging = true;
        state.dragged_block = (int)i;
        state.drag_offset = {mouse_world.x - b.position.x,
                             mouse_world.y - b.position.y};
        break;
      }
    }
  }

  if (state.dragging && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    Block &b = state.blocks[state.dragged_block];
    b.position.x = mouse_world.x - state.drag_offset.x;
    b.position.y = mouse_world.y - state.drag_offset.y;
  }

  if (state.dragging && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
    state.dragging = false;
    state.dragged_block = -1;
  }

  return play_pressed;
}

// TODO: Use actual graph, instead of the hardcoded RELU model.
void run_graph_inference(GraphState &state) {
  if (state.blocks.size() < 3)
    return;

  Block &input_block = state.blocks[0];
  Block &output_block = state.blocks[2];

  output_block.values.resize(input_block.values.size());
  for (size_t i = 0; i < input_block.values.size(); i++) {
    output_block.values[i] = std::max(0.0f, input_block.values[i]);
  }
  output_block.has_results = true;
  state.inference_ran = true;
}
