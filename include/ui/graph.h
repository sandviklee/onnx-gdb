#pragma once
#include "raylib.h"
#include <string>
#include <vector>

enum class BlockType {
  PORT_INPUT,
  PORT_OUTPUT,
  OPERATOR, // Operator node (RELU)
};

struct Block {
  BlockType type;
  std::string label; // Display name: "X", "Y", "RELU"
  Vector2 position;  // Top-left position in world space
  float width;
  float height;
  std::vector<float>
      values; // For PORT_INPUT: editable values; for PORT_OUTPUT: result values
  bool has_results; // Whether output values have been computed
};

struct InputFieldState {
  int active_block; // Index of block being edited (-1 = none)
  int active_field; // Index of field within the block (-1 = none)
  char buffer[32];  // Text editing buffer
  int cursor;       // Cursor position in buffer
};

struct GraphState {
  std::vector<Block> blocks;
  InputFieldState input_state;
  bool inference_ran;  // Whether inference has been run at least once
  bool dragging;       // Whether a block is being dragged
  int dragged_block;   // Index of block being dragged
  Vector2 drag_offset; // Offset from block origin to mouse when drag started
};

// TODO:  Initialize a pre-built X -> RELU -> Y graph
GraphState create_default_graph();

// Draw all blocks and connections on the canvas (call inside BeginMode2D)
void draw_graph(const GraphState &state);

void draw_graph_ui(const GraphState &state);

// Handle all graph interaction: input field editing, play button, block
// dragging Returns true if the Play button was pressed
bool update_graph(GraphState &state, const Camera2D &camera);

// Run inference on the graph
void run_graph_inference(GraphState &state);
