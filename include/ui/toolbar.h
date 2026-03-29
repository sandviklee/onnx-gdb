#include "raylib.h"
#include <vector>

#pragma once
#ifndef TOOLBAR_H
#define TOOLBAR_H

enum class ToolbarButtonType { POINTER, LIBRARY, DEBUG, INFERENCE };

constexpr ToolbarButtonType all_types[] = {
    ToolbarButtonType::POINTER, ToolbarButtonType::LIBRARY,
    ToolbarButtonType::DEBUG, ToolbarButtonType::INFERENCE};

class ToolbarButton {
  friend class Toolbar;

private:
  Rectangle rect;
  const char *text;
  const ToolbarButtonType type;

public:
  ToolbarButton(const ToolbarButtonType type, const float size,
                const Vector2 position);
  void draw();
};

class Toolbar {
private:
  std::vector<ToolbarButton> buttons;
  float height;
  float width;
  Vector2 position;

public:
  Toolbar(const Vector2 position, const float button_size);
  void draw();
  void handle_click();
};

#endif
