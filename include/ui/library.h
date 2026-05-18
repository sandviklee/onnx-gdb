#pragma once
#include "ir/operator.h"
#include "raylib.h"
#include <string>
#include <vector>

namespace ui {

struct LibraryEntry {
  Rectangle rect;
  const ir::OperatorSpec *spec;
};

class Library {
public:
  Library(Vector2 position, float width, float height);

  void draw();
  std::string handle_click();

private:
  Rectangle rect;
  std::vector<LibraryEntry> entries;
  float scroll_offset = 0.0f;
};

} // namespace ui
