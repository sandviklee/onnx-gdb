#include "raylib.h"
#include "ui/block.h"
#include <memory>
#include <vector>

#pragma once
#ifndef TOOLBAR_H
#define TOOLBAR_H

struct LibraryOperator {
  Rectangle rect;
  BlockDefinition &definition;
};

class Library {

private:
  Rectangle rect;
  std::vector<LibraryOperator> operators;
  float scroll_offset = 0.0f;

public:
  Library(const Vector2 position, const float width, const float height);

  void draw();
  std::string handle_click();
};

enum class ToolbarButtonType { OPEN_FILE, LIBRARY, DEBUG, INFERENCE, RESET };

constexpr ToolbarButtonType all_types[] = {
    ToolbarButtonType::OPEN_FILE, ToolbarButtonType::LIBRARY,
    ToolbarButtonType::DEBUG, ToolbarButtonType::INFERENCE,
    ToolbarButtonType::RESET};

class ToolbarButton {
  friend class Toolbar;

private:
  Rectangle rect;
  int icon_id;
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
  bool show_library = false;
  std::unique_ptr<Library> library;

  Toolbar(const Vector2 position, const float button_size);
  void draw();
  int handle_click();
};

#endif
