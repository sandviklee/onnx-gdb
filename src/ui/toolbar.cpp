#include "ui/toolbar.h"
#include "raygui.h"
#include "raylib.h"
#include "ui/block.h"

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
    }
  }

  return -1;
}

Library::Library(const Vector2 position, const float width, const float height)
    : rect({position.x, position.y, width, height}) {

  size_t amount = 1;
  size_t row = 0;
  for (auto &[name, def] : BLOCK_REGISTRY) {
    float pos_x = position.x + (LIBRARY_ITEM_SIZE + 10) * (amount % 2);
    float pos_y = position.y + (LIBRARY_ITEM_SIZE + 40) * row;

    Rectangle operator_rect = {pos_x, pos_y, LIBRARY_ITEM_SIZE,
                               LIBRARY_ITEM_SIZE};
    this->operators.push_back({operator_rect, def});
    if (amount % 2 == 0) {
      row++;
    }
    amount++;
  }
};

void Library::draw() {
  DrawRectangleRec(this->rect, LIGHTGRAY);
  DrawRectangleLinesEx(this->rect, 2, GRAY);

  auto font_size = 16;
  auto text_vector = MeasureTextEx(GetFontDefault(), "Library", font_size, 1);

  int title_x = this->rect.x + 10;
  int title_y = this->rect.y - 5;

  DrawRectangle(title_x - 4, title_y - 4, text_vector.x + 16,
                text_vector.y + 12, GRAY);
  DrawText("Library", title_x, title_y + 2, font_size, WHITE);

  for (auto op : this->operators) {
    DrawRectangle(op.rect.x, op.rect.y, op.rect.width, op.rect.height, GRAY);
    DrawText(op.definition.name.c_str(), op.rect.x + 4,
             (op.rect.y + LIBRARY_ITEM_SIZE / 2) - text_vector.y / 2, font_size,
             WHITE);
  }
}

std::string Library::handle_click() {
  if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    return "";

  Vector2 mouse = GetMousePosition();
  for (auto &op : this->operators) {
    if (CheckCollisionPointRec(mouse, op.rect)) {
      return op.definition.name;
    }
  }
  return "";
}
