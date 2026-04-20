#include "ui/toolbar.h"
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include "ui/block.h"
#include <algorithm>

ToolbarButton::ToolbarButton(const ToolbarButtonType type, const float size,
                             const Vector2 position)
    : type(type) {
  switch (type) {
  case ToolbarButtonType::POINTER:
    icon_id = ICON_CURSOR_CLASSIC;
    break;
  case ToolbarButtonType::LIBRARY:
    icon_id = ICON_FOLDER_OPEN;
    break;
  case ToolbarButtonType::DEBUG:
    icon_id = ICON_PLAYER_NEXT;
    break;
  case ToolbarButtonType::INFERENCE:
    icon_id = ICON_PLAYER_PLAY;
    break;
  }
  rect = {position.x, position.y, size, size};
}

Toolbar::Toolbar(const Vector2 position, const float button_size)
    : position(position) {
  size_t size_all_types = std::size(all_types);

  width = size_all_types * (button_size + TOOLBAR_PADDING) + TOOLBAR_PADDING;
  height = button_size + TOOLBAR_PADDING * 2;

  for (size_t i = 0; i < size_all_types; i++) {
    ToolbarButtonType type = all_types[i];
    Vector2 button_position = {
        position.x + i * (button_size + TOOLBAR_PADDING) + TOOLBAR_PADDING,
        position.y + TOOLBAR_PADDING};
    if (type == ToolbarButtonType::LIBRARY) {
      Vector2 library_position = {button_position.x + (button_size / 2) -
                                      LIBRARY_W / 2,
                                  position.y - (LIBRARY_H + button_size / 2)};
      library =
          std::make_unique<Library>(library_position, LIBRARY_W, LIBRARY_H);
      show_library = false;
    }
    buttons.push_back(ToolbarButton(type, button_size, button_position));
  }
}

void Toolbar::draw() {
  DrawRectangleRec({position.x, position.y, width, height}, LIGHTGRAY);
  for (const auto &button : buttons) {
    DrawRectangleRec(button.rect, GRAY);
    GuiSetIconScale(2);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(WHITE));
    GuiLabel(button.rect, GuiIconText(button.icon_id, NULL));
  }
  if (this->show_library)
    this->library->draw();
}

int Toolbar::handle_click() {
  if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    return -1;

  Vector2 mouse = GetMousePosition();
  for (ToolbarButton &button : buttons) {
    if (CheckCollisionPointRec(mouse, button.rect)) {
      return (int)button.type;
    } else {
      this->show_library = false;
    }
  }

  return -1;
}

static Color block_type_color(BlockType type) {
  switch (type) {
  case BlockType::MATH:
    return BLUE;
  case BlockType::ACTIVATION:
    return GREEN;
  case BlockType::LAYER:
    return VIOLET;
  case BlockType::IO:
    return GRAY;
  default:
    return LIGHTGRAY;
  }
}

static int block_type_order(BlockType type) {
  switch (type) {
  case BlockType::IO:
    return 0;
  case BlockType::MATH:
    return 1;
  case BlockType::ACTIVATION:
    return 2;
  case BlockType::LAYER:
    return 3;
  default:
    return 4;
  }
}

Library::Library(const Vector2 position, const float width, const float height)
    : rect({position.x, position.y, width, height}), scroll_offset(0.0f) {

  std::vector<std::string> names;
  for (auto &[name, def] : BLOCK_REGISTRY) {
    names.push_back(name);
  }

  std::sort(names.begin(), names.end(),
            [](const std::string &a, const std::string &b) {
              const BlockDefinition &da = BLOCK_REGISTRY.at(a);
              const BlockDefinition &db = BLOCK_REGISTRY.at(b);
              int ta = block_type_order(da.type);
              int tb = block_type_order(db.type);
              if (ta != tb)
                return ta < tb;
              return a < b;
            });

  float item_x = position.x + LIBRARY_PADDING;
  float item_w = width - LIBRARY_PADDING * 2;
  float item_y_base = position.y + LIBRARY_HEADER_H + LIBRARY_PADDING;

  for (size_t i = 0; i < names.size(); i++) {
    float pos_y = item_y_base + i * (LIBRARY_ITEM_H + LIBRARY_ITEM_GAP);
    Rectangle operator_rect = {item_x, pos_y, item_w, LIBRARY_ITEM_H};
    this->operators.push_back({operator_rect, BLOCK_REGISTRY.at(names[i])});
  }
};

void Library::draw() {
  float total_items_h = operators.size() * (LIBRARY_ITEM_H + LIBRARY_ITEM_GAP);
  float content_visible_h =
      rect.height - LIBRARY_HEADER_H - LIBRARY_PADDING * 2;
  float max_scroll = std::max(0.0f, total_items_h - content_visible_h);

  if (CheckCollisionPointRec(GetMousePosition(), rect)) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0)
      scroll_offset = Clamp(scroll_offset - wheel * 30.0f, 0.0f, max_scroll);
  }

  DrawRectangleRec(rect, {35, 35, 40, 255});
  DrawRectangleLinesEx(rect, 1.5f, {70, 70, 75, 255});

  Rectangle header_rect = {rect.x, rect.y, rect.width, LIBRARY_HEADER_H};
  DrawRectangleRec(header_rect, {25, 25, 30, 255});
  DrawText("Library", rect.x + 12, rect.y + (int)(LIBRARY_HEADER_H - 16) / 2,
           16, WHITE);

  Rectangle content_rect = {rect.x, rect.y + LIBRARY_HEADER_H, rect.width,
                            rect.height - LIBRARY_HEADER_H};
  BeginScissorMode((int)content_rect.x, (int)content_rect.y,
                   (int)content_rect.width, (int)content_rect.height);

  for (auto &op : operators) {
    Rectangle draw_rect = {op.rect.x, op.rect.y - scroll_offset, op.rect.width,
                           op.rect.height};

    if (draw_rect.y + draw_rect.height <= content_rect.y)
      continue;
    if (draw_rect.y >= content_rect.y + content_rect.height)
      break;

    bool hovered = CheckCollisionPointRec(GetMousePosition(), draw_rect) &&
                   CheckCollisionPointRec(GetMousePosition(), content_rect);

    Color item_bg = hovered ? Color{75, 75, 82, 255} : Color{55, 55, 62, 255};
    DrawRectangleRec(draw_rect, item_bg);

    Color stripe = block_type_color(op.definition.type);
    DrawRectangle((int)draw_rect.x, (int)draw_rect.y, 5, (int)draw_rect.height,
                  stripe);

    DrawText(op.definition.name.c_str(), (int)draw_rect.x + 14,
             (int)(draw_rect.y + (draw_rect.height - 16) / 2), 16, WHITE);
  }

  EndScissorMode();

  if (max_scroll > 0) {
    float bar_h = content_rect.height * (content_visible_h / total_items_h);
    float bar_y = content_rect.y +
                  (scroll_offset / max_scroll) * (content_rect.height - bar_h);
    DrawRectangle((int)(rect.x + rect.width - 6), (int)bar_y, 4, (int)bar_h,
                  {110, 110, 120, 200});
  }
}

std::string Library::handle_click() {
  if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    return "";

  Vector2 mouse = GetMousePosition();
  Rectangle content_rect = {rect.x, rect.y + LIBRARY_HEADER_H, rect.width,
                            rect.height - LIBRARY_HEADER_H};
  if (!CheckCollisionPointRec(mouse, content_rect))
    return "";

  for (auto &op : this->operators) {
    Rectangle draw_rect = {op.rect.x, op.rect.y - scroll_offset, op.rect.width,
                           op.rect.height};
    if (CheckCollisionPointRec(mouse, draw_rect)) {
      return op.definition.name;
    }
  }
  return "";
}
