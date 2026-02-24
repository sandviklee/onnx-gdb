#include "ui/toolbar.h"

void draw_toolbar(const std::vector<toolbar_button> buttons) {
  for (const auto &button : buttons) {
    DrawRectangleRec(button.rect, GRAY);
    DrawText(button.text, button.rect.x + 10, button.rect.y + 10, 20, BLACK);
  }
}

int handle_toolbar_click(const std::vector<toolbar_button> &buttons) {
  if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    return -1;
  Vector2 mouse = GetMousePosition();
  for (size_t i = 0; i < buttons.size(); i++) {
    if (CheckCollisionPointRec(mouse, buttons[i].rect))
      return i;
  }
  return -1;
}
