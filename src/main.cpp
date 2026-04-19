#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "romll/romll.h"
#include "ui/block.h"
#include "ui/graph.h"
#include "ui/toolbar.h"

void do_library_action(Graph &graph, Toolbar &toolbar, const std::string &op,
                       const size_t shape) {
  if (op.empty()) {
    return;
  }
  graph.push_block(new Block(op, graph.generate_block_label(op),
                             Vector2{400.0f, 400.0f}, shape));
  toolbar.show_library = false;
}

void do_toolbar_action(ROMLL &romll, Graph &graph, Toolbar &toolbar,
                       const int action) {
  if (action == -1) {
    return;
  }
  switch ((ToolbarButtonType)action) {
  case ToolbarButtonType::POINTER:
    break;
  case ToolbarButtonType::LIBRARY:
    toolbar.show_library = !toolbar.show_library;
  case ToolbarButtonType::DEBUG:
    break;
  case ToolbarButtonType::INFERENCE:
    romll.run_inference();
    if (graph.input_state->active_block != nullptr) {
      reset_input_state(*graph.input_state);
    }
  }
}

int main() {
  struct raylib_config config = {
      .window_height = 800,
      .window_width = 1200,
  };

  InitWindow(config.window_width, config.window_height, "ROMLL");
  SetTargetFPS(60);

  Camera2D camera = {};
  camera.zoom = 1.0f;

  size_t shape = 4; // TODO: Initialize by user
  Graph graph = Graph(shape);
  ROMLL romll = ROMLL(graph);
  size_t offset_x =
      (std::size(all_types) / 2) * (TOOLBAR_BUTTON_SIZE + TOOLBAR_PADDING);
  size_t offset_y = TOOLBAR_BUTTON_SIZE * 2;
  Toolbar toolbar = Toolbar({float(config.window_width / 2 - offset_x),
                             float(config.window_height - offset_y)},
                            TOOLBAR_BUTTON_SIZE);

  while (!WindowShouldClose()) {
    graph.update(camera);

    if (toolbar.show_library) {
      std::string library_action = toolbar.library->handle_click();
      do_library_action(graph, toolbar, library_action, shape);
    }

    int toolbar_action = toolbar.handle_click();
    do_toolbar_action(romll, graph, toolbar, toolbar_action);

    if (!graph.dragging && !graph.connection_state.active &&
        graph.input_state->active_block == nullptr &&
        (IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
         IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))) {
      Vector2 delta = GetMouseDelta();
      delta = Vector2Scale(delta, -1.0f / camera.zoom);
      camera.target = Vector2Add(camera.target, delta);
    }

    float wheel = GetMouseWheelMove();
    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
        IsKeyDown(KEY_LEFT_SUPER)) {
      if (wheel != 0) {
        camera.offset = GetMousePosition();
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        camera.target = mouseWorldPos;
        float scale = 0.05f * wheel;
        camera.zoom = Clamp(expf(logf(camera.zoom) + scale), 0.125f, 64.0f);
      }
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(camera);

    graph.draw(camera);

    EndMode2D();

    toolbar.draw();

    DrawText(
        "Left og middle mouse drag to pan. Ctrl+Scroll to zoom. Drag blocks to "
        "reposition.",
        20, 22, 18, GRAY);
    DrawCircleV(GetMousePosition(), 3, DARKGRAY);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
