#pragma once
#include "raylib.h"
#include "ui/library.h"
#include <memory>
#include <vector>

namespace ui {

enum class ToolbarButtonType { OPEN_FILE, LIBRARY, DEBUG, INFERENCE, RESET };

constexpr ToolbarButtonType all_toolbar_types[] = {
    ToolbarButtonType::OPEN_FILE, ToolbarButtonType::LIBRARY,
    ToolbarButtonType::DEBUG,     ToolbarButtonType::INFERENCE,
    ToolbarButtonType::RESET,
};

class ToolbarButton {
  friend class Toolbar;

private:
  Rectangle rect;
  int icon_id;
  const ToolbarButtonType type;

public:
  ToolbarButton(ToolbarButtonType type, float size, Vector2 position);
  void draw() const;
};

class Toolbar {
public:
  bool show_library = false;
  std::unique_ptr<Library> library;

  Toolbar(Vector2 position, float button_size);
  void draw() const;
  int handle_click();

private:
  std::vector<ToolbarButton> buttons;
  float height;
  float width;
  Vector2 position;
};

} // namespace ui
