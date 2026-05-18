#include "ui/library.h"
#include "raylib.h"
#include "raymath.h"
#include "ui/block.h"
#include <algorithm>
#include <vector>

namespace ui {

Library::Library(Vector2 position, float width, float height)
    : rect({position.x, position.y, width, height}), scroll_offset(0.0f) {
  const auto &registry = ir::operator_registry();

  std::vector<std::string> names;
  names.reserve(registry.size());
  for (const auto &[name, spec] : registry)
    names.push_back(name);

  std::sort(names.begin(), names.end(),
            [&registry](const std::string &a, const std::string &b) {
              int order_a = operator_category_order(registry.at(a).category);
              int order_b = operator_category_order(registry.at(b).category);
              if (order_a != order_b)
                return order_a < order_b;
              return a < b;
            });

  float item_x = position.x + LIBRARY_PADDING;
  float item_w = width - LIBRARY_PADDING * 2;
  float item_y_base = position.y + LIBRARY_HEADER_H + LIBRARY_PADDING;

  for (size_t i = 0; i < names.size(); i++) {
    float pos_y = item_y_base + i * (LIBRARY_ITEM_H + LIBRARY_ITEM_GAP);
    Rectangle entry_rect = {item_x, pos_y, item_w, LIBRARY_ITEM_H};
    entries.push_back({entry_rect, &registry.at(names[i])});
  }
}

void Library::draw() {
  float total_items_h = entries.size() * (LIBRARY_ITEM_H + LIBRARY_ITEM_GAP);
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

  for (const auto &entry : entries) {
    Rectangle draw_rect = {entry.rect.x, entry.rect.y - scroll_offset,
                           entry.rect.width, entry.rect.height};

    if (draw_rect.y + draw_rect.height <= content_rect.y)
      continue;
    if (draw_rect.y >= content_rect.y + content_rect.height)
      break;

    bool hovered = CheckCollisionPointRec(GetMousePosition(), draw_rect) &&
                   CheckCollisionPointRec(GetMousePosition(), content_rect);

    Color item_bg = hovered ? Color{75, 75, 82, 255} : Color{55, 55, 62, 255};
    DrawRectangleRec(draw_rect, item_bg);

    Color stripe = operator_category_color(entry.spec->category);
    DrawRectangle((int)draw_rect.x, (int)draw_rect.y, 5, (int)draw_rect.height,
                  stripe);

    DrawText(entry.spec->name.c_str(), (int)draw_rect.x + 14,
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

  for (const auto &entry : entries) {
    Rectangle draw_rect = {entry.rect.x, entry.rect.y - scroll_offset,
                           entry.rect.width, entry.rect.height};
    if (CheckCollisionPointRec(mouse, draw_rect))
      return entry.spec->name;
  }
  return "";
}

} // namespace ui
