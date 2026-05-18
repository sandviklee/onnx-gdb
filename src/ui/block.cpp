#include "ui/block.h"
#include <cstdio>

namespace ui {

Color operator_category_color(ir::OperatorCategory category) {
  switch (category) {
  case ir::OperatorCategory::MATH:
    return BLUE;
  case ir::OperatorCategory::ACTIVATION:
    return GREEN;
  case ir::OperatorCategory::LAYER:
    return VIOLET;
  case ir::OperatorCategory::IO:
    return GRAY;
  }
  return LIGHTGRAY;
}

int operator_category_order(ir::OperatorCategory category) {
  switch (category) {
  case ir::OperatorCategory::IO:
    return 0;
  case ir::OperatorCategory::MATH:
    return 1;
  case ir::OperatorCategory::ACTIVATION:
    return 2;
  case ir::OperatorCategory::LAYER:
    return 3;
  }
  return 4;
}

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

Block::Block(ir::Node *node, const Vector2 &position)
    : node(node), position(position) {
  height = calculate_height();
}

void Block::update_height() { height = calculate_height(); }

float Block::calculate_height() const {
  if (node->spec->name == "PortInput") {
    int rank = (int)node->shape_dims.size();
    float field_area = 0.0f;
    if (rank == 0) {
      field_area = FIELD_H + 2.0f * FIELD_PAD;
    } else if (rank == 1) {
      field_area = node->shape_dims[0] * (FIELD_H + FIELD_PAD) + FIELD_PAD;
    } else if (rank == 2) {
      int rows = node->shape_dims[0], cols = node->shape_dims[1];
      if (rows <= MAX_GRID_ROWS && cols <= MAX_GRID_COLS)
        field_area = rows * (GRID_CELL_H + GRID_CELL_PAD) + FIELD_PAD + 18.0f;
      else
        field_area = 24.0f;
    } else {
      field_area = 24.0f;
    }
    return IO_FIELD_START_H + field_area;
  }
  if (node->spec->name == "PortOutput" && node->has_results) {
    return BLOCK_H_BASE + (FIELD_H + FIELD_PAD) * node->values.size() +
           FIELD_PAD;
  }
  return BLOCK_H_BASE;
}

std::vector<Rectangle> Block::calculate_field_rects() const {
  std::vector<Rectangle> rects;
  if (node->spec->name != "PortInput")
    return rects;

  int rank = (int)node->shape_dims.size();
  float fx = position.x + 10.0f;
  float fw = width - 20.0f;
  float start_y = position.y + IO_FIELD_START_H;

  if (rank == 0) {
    rects.push_back({fx, start_y, fw, FIELD_H});
  } else if (rank == 1) {
    for (int i = 0; i < node->shape_dims[0]; i++)
      rects.push_back({fx, start_y + i * (FIELD_H + FIELD_PAD), fw, FIELD_H});
  } else if (rank == 2) {
    int rows = node->shape_dims[0], cols = node->shape_dims[1];
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
  return rects;
}

std::vector<Vector2> Block::calculate_output_ports() const {
  size_t num_outputs = node->spec->num_outputs;
  std::vector<Vector2> ports;
  float spacing = height / (num_outputs + 1);
  for (size_t i = 0; i < num_outputs; i++) {
    float y = position.y + spacing * (i + 1);
    ports.push_back({position.x + width, y});
  }
  return ports;
}

std::vector<Vector2> Block::calculate_input_ports() const {
  size_t num_inputs = node->spec->num_inputs;
  std::vector<Vector2> ports;
  float spacing = height / (num_inputs + 1);
  for (size_t i = 0; i < num_inputs; i++) {
    float y = position.y + spacing * (i + 1);
    ports.push_back({position.x - 2.0f, y});
  }
  return ports;
}

void Block::draw(const InputFieldState &input_state) const {
  Color block_color = operator_category_color(node->spec->category);

  DrawRectangleRounded({position.x, position.y, width, height}, 0.15f, 6,
                       block_color);
  DrawRectangleRoundedLinesEx({position.x, position.y, width, height}, 0.15f, 6,
                              2.0f, BLACK);

  int font_size = 22;
  int text_w = MeasureText(node->label.c_str(), font_size);
  DrawText(node->label.c_str(), position.x + (width - text_w) / 2.0f,
           position.y + 8, font_size, BLACK);

  if (node->spec->num_inputs > 0) {
    auto in_ports = calculate_input_ports();
    for (size_t i = 0; i < node->spec->num_inputs; i++) {
      DrawCircleV(in_ports[i], PORT_RADIUS, DARKGRAY);
      DrawCircleLines(in_ports[i].x, in_ports[i].y, PORT_RADIUS, BLACK);
    }
  }

  if (node->spec->num_outputs > 0) {
    auto out_ports = calculate_output_ports();
    for (size_t i = 0; i < node->spec->num_outputs; i++) {
      DrawCircleV(out_ports[i], PORT_RADIUS, DARKGRAY);
      DrawCircleLines(out_ports[i].x, out_ports[i].y, PORT_RADIUS, BLACK);
    }
  }

  if (node->spec->name == "PortInput") {
    char shape_label[64];
    format_shape_label(node->shape_dims, shape_label, sizeof(shape_label));
    int lbl_w = MeasureText(shape_label, 14);
    DrawText(shape_label, position.x + (width - lbl_w) / 2.0f, position.y + 36,
             14, DARKGRAY);

    int rank = (int)node->shape_dims.size();
    auto rects = calculate_field_rects();

    if (rank == 2 && !rects.empty()) {
      int cols = node->shape_dims[1];
      float cell_w = (width - 20.0f) / cols;
      for (int c = 0; c < cols && c <= MAX_GRID_COLS; c++) {
        char hdr[8];
        snprintf(hdr, sizeof(hdr), "%d", c);
        float hx = position.x + 10.0f + c * cell_w +
                   (cell_w - 2.0f - MeasureText(hdr, 12)) / 2.0f;
        DrawText(hdr, hx, position.y + IO_FIELD_START_H - 14, 12, DARKGRAY);
      }
    }

    for (size_t i = 0; i < rects.size() && i < node->values.size(); i++) {
      Rectangle fr = rects[i];
      bool is_active = (input_state.active_node == node &&
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
        snprintf(val_text, sizeof(val_text), "%.2f", node->values[i]);
        DrawText(val_text, fr.x + text_pad, ty, field_font, DARKGRAY);
      }
    }

    if ((rank == 2 && rects.empty()) || rank >= 3) {
      int total = 1;
      for (int d : node->shape_dims)
        total *= d;
      char info[64];
      snprintf(info, sizeof(info), "%d values", total);
      int iw = MeasureText(info, 13);
      DrawText(info, position.x + (width - iw) / 2.0f,
               position.y + IO_FIELD_START_H, 13, GRAY);
    }
  }

  if (node->spec->name == "PortOutput" && node->has_results) {
    float field_y = position.y + FIELD_START_H;
    for (size_t i = 0; i < node->values.size(); i++) {
      float fx = position.x + 10.0f;
      float fw = width - 20.0f;

      DrawRectangleRounded({fx, field_y, fw, FIELD_H}, 0.3f, 4, YELLOW);
      DrawRectangleRoundedLinesEx({fx, field_y, fw, FIELD_H}, 0.3f, 4, 1.0f,
                                  DARKGRAY);

      char val_text[FLOAT_BUFFER_SIZE];
      snprintf(val_text, sizeof(val_text), "%.4f", node->values[i]);
      DrawText(val_text, fx + 6, field_y + 5, 18, BLACK);

      field_y += FIELD_H + FIELD_PAD;
    }
  }
}

} // namespace ui
