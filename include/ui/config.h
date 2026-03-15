#include "raylib.h"

static constexpr int FLOAT_BUFFER_SIZE = 32;

static constexpr float BLOCK_W = 180.0f;
static constexpr float BLOCK_H_BASE = 60.0f;
static constexpr float FIELD_PAD = 4.0f;
static constexpr float FIELD_H = 28.0f;
static constexpr float FIELD_START_H = 38.0f;
static constexpr float PORT_RADIUS = 8.0f;
static constexpr float WIRE_THICK = 3.0f;

static const Color COLOR_PORT_INPUT = {100, 180, 255, 255};
static const Color COLOR_RELU = {255, 180, 80, 255};
static const Color COLOR_PORT_OUTPUT = {100, 220, 130, 255};
static const Color COLOR_FIELD_BG = {240, 240, 240, 255};
static const Color COLOR_FIELD_ACTIVE = {255, 255, 220, 255};
static const Color COLOR_WIRE = {80, 80, 80, 255};
static const Color COLOR_INFERENCE = {80, 200, 80, 255};
static const Color COLOR_INFERENCE_HOVER = {60, 240, 60, 255};
static const Color COLOR_RESULT_BG = {220, 245, 220, 255};

struct raylib_config {
  int window_height;
  int window_width;
};
