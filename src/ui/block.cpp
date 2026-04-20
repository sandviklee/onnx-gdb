#include "ui/block.h"
#include <cstdio>
#include <cstring>

Block::Block(const std::string &name, const std::string &label,
             const Vector2 &position, const int shape)
    : definition(&BLOCK_REGISTRY.at(name)), position(position), label(label),
      inputs(), outputs() {
  for (int i = 0; i < shape; i++) {
    values.push_back(0.0f);
  }
  width = BLOCK_W;
  height = calculate_height();
  has_results = false;
}

float Block::calculate_height() {
  if (this->definition->name == "PortInput") {
    return BLOCK_H_BASE + (FIELD_H + FIELD_PAD + 2.0f) * this->values.size() +
           FIELD_PAD;
  }
  if (this->definition->name == "PortOutput" && this->has_results) {
    return BLOCK_H_BASE + (FIELD_H + FIELD_PAD) * this->values.size() +
           FIELD_PAD;
  }
  return BLOCK_H_BASE;
}

std::vector<Vector2> Block::calculate_output_ports() {
  size_t outputs = this->definition->num_outputs;
  std::vector<Vector2> ports;
  float spacing = this->height / (outputs + 1);
  for (size_t i = 0; i < outputs; i++) {
    float y = this->position.y + spacing * (i + 1);
    ports.push_back({this->position.x + this->width, y});
  }
  return ports;
}

std::vector<Vector2> Block::calculate_input_ports() {
  size_t inputs = this->definition->num_inputs;
  std::vector<Vector2> ports;
  float spacing = this->height / (inputs + 1);
  for (size_t i = 0; i < inputs; i++) {
    float y = this->position.y + spacing * (i + 1);
    ports.push_back({this->position.x - 2.0f, y});
  }
  return ports;
}

void Block::draw(const InputFieldState &input_state) {
  float h = this->height;
  Color block_color;

  switch (this->definition->type) {
  case BlockType::IO:
    block_color = GRAY;
    break;
  case BlockType::MATH:
    block_color = BLUE;
    break;
  case BlockType::ACTIVATION:
    block_color = GREEN;
    break;
  case BlockType::LAYER:
    block_color = VIOLET;
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

  if (definition->num_inputs > 0) {
    auto inputs = calculate_input_ports();
    for (size_t i = 0; i < definition->num_inputs; ++i) {
      Vector2 ip = inputs[i];
      DrawCircleV(ip, PORT_RADIUS, DARKGRAY);
      DrawCircleLines(ip.x, ip.y, PORT_RADIUS, BLACK);
    }
  }

  if (definition->num_outputs > 0) {
    auto outputs = calculate_output_ports();
    for (size_t i = 0; i < definition->num_outputs; ++i) {
      Vector2 op = outputs[i];
      DrawCircleV(op, PORT_RADIUS, DARKGRAY);
      DrawCircleLines(op.x, op.y, PORT_RADIUS, BLACK);
    }
  }

  if (definition->name == "PortInput") {
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

  if (definition->name == "PortOutput" && this->has_results) {
    float field_y = this->position.y + FIELD_START_H;
    for (size_t i = 0; i < this->values.size(); i++) {
      float fx = this->position.x + 10.0f;
      float fw = this->width - 20.0f;

      DrawRectangleRounded({fx, field_y, fw, FIELD_H}, 0.3f, 4, YELLOW);
      DrawRectangleRoundedLinesEx({fx, field_y, fw, FIELD_H}, 0.3f, 4, 1.0f,
                                  DARKGRAY);

      char val_text[FLOAT_BUFFER_SIZE];
      snprintf(val_text, sizeof(val_text), "%.4f", this->values[i]);
      DrawText(val_text, fx + 6, field_y + 5, 18, BLACK);

      field_y += FIELD_H + FIELD_PAD;
    }
  }
}
