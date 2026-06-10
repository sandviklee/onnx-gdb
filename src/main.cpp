#ifdef ROMLL_ONNXMLIR_AVAILABLE
#endif
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "ui/filedlg.h"
#include "ui/graph.h"
#include "ui/toolbar.h"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

// ---- Backend selection ----
// Remember  -DENABLE_ONNXMLIR_BACKEND=ON if OnnxMlirBAckend is desired.
#include "backend/backend.h"
using ActiveBackend = backend::OrtBackend;
// #include "backend/onnxmlir_backend.h"
// using ActiveBackend = backend::OnnxMlirBackend;
// ---------------------------

static void do_library_action(ui::UIGraph &ui_graph, ui::Toolbar &toolbar,
                              const std::string &op) {
  if (op.empty())
    return;
  ui::Block *block = ui_graph.add_block(op, Vector2{400.0f, 400.0f});
  if (op == "PortInput")
    ui_graph.open_shape_popup(block);
  toolbar.show_library = false;
}

static void load_onnx(std::string path, ui::UIGraph &ui_graph) {
  std::string msg;
  if (ui_graph.ir_graph.load_onnx_file(path, msg)) {
    ui_graph.rebuild_from_ir();
    if (msg.empty())
      ui_graph.push_notification("Model loaded successfully", false);
    else
      ui_graph.push_notification("Loaded (warnings): " + msg, false);
  } else {
    ui_graph.push_notification("Import failed: " + msg, true);
  }
}

static void do_toolbar_action(ActiveBackend &backend, ui::UIGraph &ui_graph,
                              ui::Toolbar &toolbar, int action) {
  if (action == -1)
    return;
  switch ((ui::ToolbarButtonType)action) {
  case ui::ToolbarButtonType::OPEN_FILE: {
    std::string path = open_onnx_file_dialog();
    if (!path.empty()) {
      load_onnx(path, ui_graph);
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
        backend.run_debug_inference();
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
      backend.run_inference();
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
  case ui::ToolbarButtonType::DOWNLOAD: {
    std::time_t t =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm = *std::localtime(&t);
    std::ostringstream stamp;
    stamp << std::put_time(&tm, "%Y-%m-%d_%H%M%S");
    std::filesystem::path dir = "models";
    std::filesystem::create_directories(dir);
    std::filesystem::path path = dir / (stamp.str() + ".onnx");

    const std::string &bytes = ui_graph.ir_graph.current_onnx_bytes();
    std::ofstream out(path, std::ios::binary);
    out.write(bytes.data(), (std::streamsize)bytes.size());
    if (out)
      ui_graph.push_notification("Saved model to " + path.string(), false);
    else
      ui_graph.push_notification("Failed to save model to " + path.string(),
                                 true);
    break;
  }
  }
}

int main() {
  struct raylib_config config = {
      .window_height = 800,
      .window_width = 1200,
  };

  InitWindow(config.window_width, config.window_height, "ROMLL");
  SetTargetFPS(0);
  SetConfigFlags(0);

  Camera2D camera = {};
  camera.zoom = 1.0f;

  ui::UIGraph ui_graph;
  ActiveBackend backend(ui_graph.ir_graph,
                        [&ui_graph](const std::string &msg, bool is_error) {
                          ui_graph.push_notification(msg, is_error);
                        });

  size_t offset_x = (std::size(ui::all_toolbar_types) / 2) *
                    (TOOLBAR_BUTTON_SIZE + TOOLBAR_PADDING);
  size_t offset_y = TOOLBAR_BUTTON_SIZE * 2;
  ui::Toolbar toolbar({float(config.window_width / 2 - offset_x),
                       float(config.window_height - offset_y)},
                      TOOLBAR_BUTTON_SIZE);

  // load_onnx("../../models/premade/500.onnx", ui_graph);
  // const int frame_count = 1000;
  // int total_ms = 0;
  while (!WindowShouldClose()) {
    // for (int i = 0; i < frame_count; i++) {
    auto t0 = std::chrono::steady_clock::now();
    ui_graph.update(camera);

    if (!ui_graph.popup_active()) {
      if (toolbar.show_library) {
        std::string library_action = toolbar.library->handle_click();
        do_library_action(ui_graph, toolbar, library_action);
      }

      int toolbar_action = toolbar.handle_click();
      do_toolbar_action(backend, ui_graph, toolbar, toolbar_action);

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
    // camera.target.x += 4.0f;
    // camera.zoom += sinf(GetTime()) * 0.001f;

    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode2D(camera);

    ui_graph.draw(camera);

    EndMode2D();

    ui_graph.draw_wire_tooltips(camera);
    toolbar.draw();
    ui_graph.draw_popup();
    ui_graph.draw_attr_popup();
    ui_graph.draw_notifications();

    std::string current_backend = "Execution backend: " + backend.get_name();
    DrawText(current_backend.c_str(), 20, 22, 16, RED);
    DrawText("Left/middle drag to pan. Ctrl+Scroll to zoom. "
             "Double-click PortInput to configure shape.",
             20, 42, 16, GRAY);
    DrawCircleV(GetMousePosition(), 3, DARKGRAY);

    EndDrawing();
    // auto t1 = std::chrono::steady_clock::now();
    // total_ms += std::chrono::duration<double, std::milli>(t1 - t0).count();
    // return 0;
  }

  // printf("avg frame time: %.6f ms\n", total_ms / frame_count);
  // printf("avg FPS: %.2f\n", 1000.0 * frame_count / total_ms);

  CloseWindow();
  return 0;
}
