#include "ui/block.h"
#include <cstdio>
#include <cstring>

Block::Block(const BlockType &type, const Vector2 &position, const int shape)
    : position(position), type(type) {
  for (int i = 0; i < shape; i++) {
    values.push_back(0.0f);
  }
  width = BLOCK_W;
  height = calculate_height();
  has_results = false;

  switch (this->type) {
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
    return BLOCK_H_BASE + (FIELD_H + FIELD_PAD + 2.0f) * this->values.size() +
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
