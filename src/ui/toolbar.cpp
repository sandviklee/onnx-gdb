#include "ui/toolbar.h"
#include "raylib.h"
#include "ui/block.h"
#include <iostream>

ToolbarButton::ToolbarButton(const ToolbarButtonType type, const float size,
                             const Vector2 position)
    : type(type) {
  switch (type) {
  case ToolbarButtonType::POINTER:
    text = "P";
    break;
  case ToolbarButtonType::LIBRARY:
    text = "L";
    break;
  case ToolbarButtonType::DEBUG:
    text = "D";
    break;
  case ToolbarButtonType::INFERENCE:
    text = "I";
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
    buttons.push_back(ToolbarButton(type, button_size, button_position));
  }
}

void Toolbar::draw() {
  DrawRectangleRec({position.x, position.y, width, height}, LIGHTGRAY);
  for (const auto &button : buttons) {
    DrawRectangleRec(button.rect, GRAY);
    DrawText(button.text, (button.rect.x - 5) + button.rect.width / 2,
             (button.rect.y - 10) + button.rect.height / 2, 20, BLACK);
  }
}

void Toolbar::handle_click() {
  if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    return;

  Vector2 mouse = GetMousePosition();
  for (ToolbarButton &button : buttons) {
    if (CheckCollisionPointRec(mouse, button.rect)) {
      std::cout << "Clicked on button: " << button.text << std::endl;
    }
  }
}
