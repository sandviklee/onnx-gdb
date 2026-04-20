#include "ui/block.h"
#include <cstdio>
#include <cstring>

static void format_shape_label(const std::vector<int> &dims, char *buf,
                               int buf_size) {
  if (dims.empty())
    snprintf(buf, buf_size, "scalar");
  else if (dims.size() == 1)
    snprintf(buf, buf_size, "vec  [%d]", dims[0]);
  else if (dims.size() == 2)
    snprintf(buf, buf_size, "mat  [%dx%d]", dims[0], dims[1]);
  else
    snprintf(buf, buf_size, "tensor  [%dx%dx%d]", dims[0], dims[1], dims[2]);
}

Block::Block(const std::string &name, const std::string &label,
             const Vector2 &position, const int shape)
    : definition(&BLOCK_REGISTRY.at(name)), position(position), label(label),
      inputs(), outputs() {
  if (shape > 0) {
    shape_dims = {shape};
    for (int i = 0; i < shape; i++)
      values.push_back(0.0f);
  }
  width = BLOCK_W;
  height = calculate_height();
  has_results = false;
}

float Block::calculate_height() {
  if (this->definition->name == "PortInput") {
    int rank = (int)shape_dims.size();
    float field_area = 0.0f;
    if (rank == 0) {
      field_area = FIELD_H + 2.0f * FIELD_PAD;
    } else if (rank == 1) {
      field_area = shape_dims[0] * (FIELD_H + FIELD_PAD) + FIELD_PAD;
    } else if (rank == 2) {
      int rows = shape_dims[0], cols = shape_dims[1];
      if (rows <= MAX_GRID_ROWS && cols <= MAX_GRID_COLS)
        field_area = rows * (GRID_CELL_H + GRID_CELL_PAD) + FIELD_PAD + 18.0f;
      else
        field_area = 24.0f;
    } else {
      field_area = 24.0f;
    }
    return IO_FIELD_START_H + field_area;
  }
  if (this->definition->name == "PortOutput" && this->has_results) {
    return BLOCK_H_BASE + (FIELD_H + FIELD_PAD) * this->values.size() +
           FIELD_PAD;
  }
  return BLOCK_H_BASE;
}

std::vector<Rectangle> Block::calculate_field_rects() const {
  std::vector<Rectangle> rects;
  if (definition->name != "PortInput")
    return rects;

  int rank = (int)shape_dims.size();
  float fx = position.x + 10.0f;
  float fw = width - 20.0f;
  float start_y = position.y + IO_FIELD_START_H;

  if (rank == 0) {
    rects.push_back({fx, start_y, fw, FIELD_H});
  } else if (rank == 1) {
    for (int i = 0; i < shape_dims[0]; i++) {
      rects.push_back({fx, start_y + i * (FIELD_H + FIELD_PAD), fw, FIELD_H});
    }
  } else if (rank == 2) {
    int rows = shape_dims[0], cols = shape_dims[1];
    if (rows <= MAX_GRID_ROWS && cols <= MAX_GRID_COLS) {
      float cell_w = fw / cols;
      for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
          float cx = fx + c * cell_w;
          float cy = start_y + r * (GRID_CELL_H + GRID_CELL_PAD);
          rects.push_back({cx, cy, cell_w - 2.0f, GRID_CELL_H});
        }
      }
    }
  }
  // rank 3+: no inline fields
  return rects;
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
    char shape_label[64];
    format_shape_label(shape_dims, shape_label, sizeof(shape_label));
    int lbl_w = MeasureText(shape_label, 14);
    DrawText(shape_label, this->position.x + (this->width - lbl_w) / 2.0f,
             this->position.y + 36, 14, DARKGRAY);

    int rank = (int)shape_dims.size();
    auto rects = calculate_field_rects();

    if (rank == 2 && !rects.empty()) {
      int cols = shape_dims[1];
      float cell_w = (this->width - 20.0f) / cols;
      for (int c = 0; c < cols && c <= MAX_GRID_COLS; c++) {
        char hdr[8];
        snprintf(hdr, sizeof(hdr), "%d", c);
        float hx = this->position.x + 10.0f + c * cell_w +
                   (cell_w - 2.0f - MeasureText(hdr, 12)) / 2.0f;
        DrawText(hdr, hx, this->position.y + IO_FIELD_START_H - 14, 12,
                 DARKGRAY);
      }
    }

    for (size_t i = 0; i < rects.size() && i < this->values.size(); i++) {
      Rectangle fr = rects[i];
      bool is_active = (input_state.active_block == this &&
                        input_state.active_field == (int)i);

      float corner = (rank == 2) ? 0.15f : 0.3f;
      int field_font = (rank == 2) ? 12 : 18;
      int text_pad = (rank == 2) ? 3 : 6;

      DrawRectangleRounded(fr, corner, 4,
                           is_active ? COLOR_FIELD_ACTIVE : COLOR_FIELD_BG);
      DrawRectangleRoundedLinesEx(fr, corner, 4, 1.0f,
                                  is_active ? RED : DARKGRAY);

      float ty = fr.y + (fr.height - field_font) / 2.0f;
      if (is_active) {
        DrawText(input_state.buffer, fr.x + text_pad, ty, field_font, BLACK);
        if (((int)(GetTime() * 2)) % 2 == 0) {
          int cur_x =
              fr.x + text_pad + MeasureText(input_state.buffer, field_font);
          DrawLine(cur_x, fr.y + 2, cur_x, fr.y + fr.height - 2, RED);
        }
      } else {
        char val_text[FLOAT_BUFFER_SIZE];
        snprintf(val_text, sizeof(val_text), "%.2f", this->values[i]);
        DrawText(val_text, fr.x + text_pad, ty, field_font, DARKGRAY);
      }
    }

    // Info line for large / rank3 tensors
    if ((rank == 2 && rects.empty()) || rank >= 3) {
      int total = 1;
      for (int d : shape_dims)
        total *= d;
      char info[64];
      snprintf(info, sizeof(info), "%d values", total);
      int iw = MeasureText(info, 13);
      DrawText(info, this->position.x + (this->width - iw) / 2.0f,
               this->position.y + IO_FIELD_START_H, 13, GRAY);
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
