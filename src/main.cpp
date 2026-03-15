#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "romll/romll.h"
#include "ui/config.h"
#include "ui/graph.h"
#include <onnxruntime/onnxruntime_cxx_api.h>

// void run_inference_with_romll(Graph &graph) {
//   if (!graph.ready())
//     return;
//   // TODO: Should return error code and show error in GUI.
//
//   Block &input_block = *state.blocks.front(); // TODO: Fix this cursed ass
//   shit Block &output_block = *state.blocks.back();
//
//   std::vector<float> input_data = input_block.values;
//   std::vector<int64_t> input_shape = {(int64_t)input_data.size()};
//
//   try {
//     char input_name[] = "X";
//     char output_name[] = "Y";
//     ROMLL romll(state, input_data, input_shape, {input_name}, {output_name});
//
//     Ort::RunOptions run_options{nullptr};
//     std::vector<Ort::Value> output = romll.run_model(run_options);
//     Ort::TensorTypeAndShapeInfo info = output[0].GetTensorTypeAndShapeInfo();
//     std::vector<int64_t> shape = info.GetShape();
//     float *data = output[0].GetTensorMutableData<float>();
//     int64_t size = input_shape[0];
//
//     output_block.values.resize(size);
//     for (int64_t i = 0; i < size; i++) {
//       output_block.values[i] = data[i];
//     }
//     output_block.has_results = true;
//     state.inference_ran = true;
//
//     std::cout << "Inference output: [";
//     for (int64_t i = 0; i < size; i++) {
//       std::cout << data[i] << (i < size - 1 ? ", " : "");
//     }
//     std::cout << "]" << std::endl;
//   } catch (const Ort::Exception &e) {
//     std::cerr << "ONNX Runtime error: " << e.what() << std::endl;
//   }
// }

int main() {
  struct raylib_config config = {
      .window_height = 800,
      .window_width = 1200,
  };

  InitWindow(config.window_width, config.window_height, "ROMLL");
  SetTargetFPS(60);

  Camera2D camera = {};
  camera.zoom = 1.0f;

  Graph graph = Graph(4);

  while (!WindowShouldClose()) {
    bool inference_pressed = graph.update(camera);

    // if (inference_pressed) {
    //   if (graph.input_state.active_block >= 0) {
    //     Block &b = *graph.blocks[graph.input_state.active_block];
    //     float val = (float)atof(graph.input_state.buffer);
    //     b.values[graph.input_state.active_field] = val;
    //     graph.input_state.active_block = -1;
    //     graph.input_state.active_field = -1;
    //   }
    //   run_inference_with_romll(graph);
    // }
    //
    if (!graph.dragging && graph.input_state->active_block == nullptr &&
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
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        camera.offset = GetMousePosition();
        camera.target = mouseWorldPos;
        float scale = 0.1f * wheel;
        camera.zoom = Clamp(expf(logf(camera.zoom) + scale), 0.125f, 64.0f);
      }
    }

    BeginDrawing();
    ClearBackground(RAYWHITE);

    BeginMode2D(camera);

    rlPushMatrix();
    rlTranslatef(0, 25 * 50, 0);
    rlRotatef(90, 1, 0, 0);
    DrawGrid(100, 50);
    rlPopMatrix();

    graph.draw();

    EndMode2D();

    draw_ui(graph);

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
