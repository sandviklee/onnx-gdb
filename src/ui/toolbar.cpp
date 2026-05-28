#include "ui/toolbar.h"
#include "raygui.h"
#include "raylib.h"
#include "ui/config.h"

namespace ui {

ToolbarButton::ToolbarButton(ToolbarButtonType type, float size,
                             Vector2 position)
    : type(type) {
  switch (type) {
  case ToolbarButtonType::OPEN_FILE:
    icon_id = ICON_FILE_OPEN;
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
  case ToolbarButtonType::RESET:
    icon_id = ICON_REDO;
    break;
  case ToolbarButtonType::DOWNLOAD:
    icon_id = ICON_ARROW_DOWN_FILL;
    break;
  }
  rect = {position.x, position.y, size, size};
}

Toolbar::Toolbar(Vector2 position, float button_size) : position(position) {
  size_t num_types = std::size(all_toolbar_types);
  width = num_types * (button_size + TOOLBAR_PADDING) + TOOLBAR_PADDING;
  height = button_size + TOOLBAR_PADDING * 2;

  for (size_t i = 0; i < num_types; i++) {
    ToolbarButtonType type = all_toolbar_types[i];
    Vector2 button_position = {
        position.x + i * (button_size + TOOLBAR_PADDING) + TOOLBAR_PADDING,
        position.y + TOOLBAR_PADDING,
    };
    if (type == ToolbarButtonType::LIBRARY) {
      Vector2 library_position = {
          button_position.x + (button_size / 2) - LIBRARY_WIDTH / 2,
          position.y - (LIBRARY_H + button_size / 2),
      };
      library = std::make_unique<Library>(library_position, LIBRARY_WIDTH,
                                          LIBRARY_HEIGHT);
      show_library = false;
    }
    buttons.push_back(ToolbarButton(type, button_size, button_position));
  }
}

void Toolbar::draw() const {
  DrawRectangleRec({position.x, position.y, width, height}, LIGHTGRAY);
  for (const auto &button : buttons) {
    DrawRectangleRec(button.rect, GRAY);
    GuiSetIconScale(2);
    GuiSetStyle(DEFAULT, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
    GuiSetStyle(DEFAULT, TEXT_COLOR_NORMAL, ColorToInt(WHITE));
    GuiLabel(button.rect, GuiIconText(button.icon_id, NULL));
  }
  if (show_library)
    library->draw();
}

int Toolbar::handle_click() {
  if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
    return -1;

  Vector2 mouse = GetMousePosition();
  for (const ToolbarButton &button : buttons) {
    if (CheckCollisionPointRec(mouse, button.rect))
      return (int)button.type;
    else
      show_library = false;
  }
  return -1;
}

} // namespace ui
