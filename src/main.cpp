#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "romll/romll.h"
#include "ui/block.h"
#include "ui/graph.h"
#include "ui/toolbar.h"
#include <onnxruntime_cxx_api.h>

int main() {
  auto start = std::chrono::high_resolution_clock::now();

  struct raylib_config config = {
      .window_height = 800,
      .window_width = 1200,
  };

  InitWindow(config.window_width, config.window_height, "ROMLL");
  SetTargetFPS(60);

  Camera2D camera = {};
  camera.zoom = 1.0f;

  Graph graph = Graph(4);
  ROMLL romll = ROMLL(graph);
  size_t offset_x =
      (std::size(all_types) / 2) * (TOOLBAR_BUTTON_SIZE + TOOLBAR_PADDING);
  size_t offset_y = TOOLBAR_BUTTON_SIZE * 2;
  Toolbar toolbar = Toolbar({float(config.window_width / 2 - offset_x),
                             float(config.window_height - offset_y)},
                            TOOLBAR_BUTTON_SIZE);

  auto ready = std::chrono::high_resolution_clock::now();
  printf("Startup: %.2f ms\n",
         std::chrono::duration<double, std::milli>(ready - start).count());

  // while (!WindowShouldClose()) {
  //   bool inference_pressed = graph.update(camera);
  //   toolbar.handle_click();
  //
  //   if (inference_pressed) {
  //     romll.run_inference();
  //
  //     if (graph.input_state->active_block != nullptr) {
  //       reset_input_state(*graph.input_state);
  //     }
  //   }
  //   if (!graph.dragging && !graph.connection_state.active &&
  //       graph.input_state->active_block == nullptr &&
  //       (IsMouseButtonDown(MOUSE_BUTTON_LEFT) ||
  //        IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))) {
  //     Vector2 delta = GetMouseDelta();
  //     delta = Vector2Scale(delta, -1.0f / camera.zoom);
  //     camera.target = Vector2Add(camera.target, delta);
  //   }
  //
  //   float wheel = GetMouseWheelMove();
  //   if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL) ||
  //       IsKeyDown(KEY_LEFT_SUPER)) {
  //     if (wheel != 0) {
  //       Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(),
  //       camera); camera.offset = GetMousePosition(); camera.target =
  //       mouseWorldPos; float scale = 0.1f * wheel; camera.zoom =
  //       Clamp(expf(logf(camera.zoom) + scale), 0.125f, 64.0f);
  //     }
  //   }
  //
  //   BeginDrawing();
  //   ClearBackground(RAYWHITE);
  //   BeginMode2D(camera);
  //
  //   rlPushMatrix();
  //   rlTranslatef(0, 25 * 50, 0);
  //   rlRotatef(90, 1, 0, 0);
  //   DrawGrid(100, 50);
  //   rlPopMatrix();
  //
  //   graph.draw(camera);
  //
  //   EndMode2D();
  //
  //   toolbar.draw();
  //   draw_ui(graph);
  //
  //   DrawText(
  //       "Left og middle mouse drag to pan. Ctrl+Scroll to zoom. Drag blocks
  //       to " "reposition.", 20, 22, 18, GRAY);
  //
  //   DrawCircleV(GetMousePosition(), 3, DARKGRAY);
  //
  //   EndDrawing();
  // }

  // CloseWindow();
  return 0;
}
