#include "backend/backend.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "ui/filedlg.h"
#include "ui/graph.h"
#include "ui/toolbar.h"

static void do_library_action(ui::UIGraph &ui_graph, ui::Toolbar &toolbar,
                              const std::string &op) {
  if (op.empty())
    return;
  ui::Block *block = ui_graph.add_block(op, Vector2{400.0f, 400.0f});
  if (op == "PortInput")
    ui_graph.open_shape_popup(block);
  toolbar.show_library = false;
}

static void do_toolbar_action(backend::ROMLL &romll, ui::UIGraph &ui_graph,
                              ui::Toolbar &toolbar, int action) {
  if (action == -1)
    return;
  switch ((ui::ToolbarButtonType)action) {
  case ui::ToolbarButtonType::OPEN_FILE: {
    std::string path = open_onnx_file_dialog();
    if (!path.empty()) {
      std::string msg;
      if (romll.load_onnx_file(path, msg)) {
        ui_graph.rebuild_from_ir();
        if (msg.empty())
          ui_graph.push_notification("Model loaded successfully", false);
        else
          ui_graph.push_notification("Loaded (warnings): " + msg, false);
      } else {
        ui_graph.push_notification("Import failed: " + msg, true);
      }
    }
    toolbar.show_library = false;
    break;
  }
  case ui::ToolbarButtonType::LIBRARY:
    toolbar.show_library = !toolbar.show_library;
    break;
  case ui::ToolbarButtonType::DEBUG:
    if (ui_graph.debug_mode) {
      ui_graph.disable_debug();
    } else {
      try {
        romll.run_debug_inference();
        ui_graph.debug_mode = true;
        ui_graph.push_notification("Debug: wire values visible", false);
      } catch (const std::exception &e) {
        ui_graph.push_notification(std::string("Debug error: ") + e.what(),
                                   true);
      }
    }
    break;
  case ui::ToolbarButtonType::INFERENCE:
    try {
      romll.run_inference();
      ui_graph.inference_ran = true;
    } catch (const std::exception &e) {
      ui_graph.push_notification(std::string("Inference error: ") + e.what(),
                                 true);
    }
    if (ui_graph.input_state->active_node != nullptr)
      ui::reset_input_state(*ui_graph.input_state);
    break;
  case ui::ToolbarButtonType::RESET:
    ui_graph.clear();
    ui_graph.push_notification("Graph cleared", false);
    break;
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

  ui::UIGraph ui_graph(4);
  backend::ROMLL romll(ui_graph.ir_graph,
                       [&ui_graph](const std::string &msg, bool is_error) {
                         ui_graph.push_notification(msg, is_error);
                       });

  size_t offset_x = (std::size(ui::all_toolbar_types) / 2) *
                    (TOOLBAR_BUTTON_SIZE + TOOLBAR_PADDING);
  size_t offset_y = TOOLBAR_BUTTON_SIZE * 2;
  ui::Toolbar toolbar({float(config.window_width / 2 - offset_x),
                       float(config.window_height - offset_y)},
                      TOOLBAR_BUTTON_SIZE);

  while (!WindowShouldClose()) {
    ui_graph.update(camera);

    if (!ui_graph.popup_active()) {
      if (toolbar.show_library) {
        std::string library_action = toolbar.library->handle_click();
        do_library_action(ui_graph, toolbar, library_action);
      }

      int toolbar_action = toolbar.handle_click();
      do_toolbar_action(romll, ui_graph, toolbar, toolbar_action);

      if (!ui_graph.dragging && !ui_graph.connection_state.active &&
          ui_graph.input_state->active_node == nullptr &&
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
          Vector2 mouse_world_pos =
              GetScreenToWorld2D(GetMousePosition(), camera);
          camera.offset = GetMousePosition();
          camera.target = mouse_world_pos;
          float scale = 0.1f * wheel;
          camera.zoom = Clamp(expf(logf(camera.zoom) + scale), 0.125f, 64.0f);
        }
      }
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(camera);

    ui_graph.draw(camera);

    EndMode2D();

    ui_graph.draw_wire_tooltips(camera);
    toolbar.draw();
    ui_graph.draw_popup();
    ui_graph.draw_notifications();

    DrawText("Left/middle drag to pan. Ctrl+Scroll to zoom. "
             "Double-click PortInput to configure shape.",
             20, 22, 16, GRAY);
    DrawCircleV(GetMousePosition(), 3, DARKGRAY);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
