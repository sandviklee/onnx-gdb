#pragma once
#include "raylib.h"

static constexpr int TOOLBAR_PADDING = 10;
static constexpr int TOOLBAR_BUTTON_SIZE = 50;

static constexpr int FLOAT_BUFFER_SIZE = 32;

static constexpr float BLOCK_W = 180.0f;
static constexpr float BLOCK_H_BASE = 100.0f;
static constexpr float FIELD_PAD = 4.0f;
static constexpr float FIELD_H = 28.0f;
static constexpr float FIELD_START_H = 38.0f;
static constexpr float IO_FIELD_START_H = 70.0f;
static constexpr float GRID_CELL_H = 22.0f;
static constexpr float GRID_CELL_PAD = 2.0f;
static constexpr int MAX_GRID_COLS = 5;
static constexpr int MAX_GRID_ROWS = 6;
static constexpr float PORT_RADIUS = 8.0f;
static constexpr float WIRE_THICK = 3.0f;

static constexpr float LIBRARY_HEIGHT = 420.0f;
static constexpr float LIBRARY_WIDTH = 260.0f;
static constexpr float LIBRARY_HEADER_H = 44.0f;
static constexpr float LIBRARY_ITEM_H = 38.0f;
static constexpr float LIBRARY_ITEM_GAP = 4.0f;
static constexpr float LIBRARY_PADDING = 10.0f;

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

static constexpr float LIBRARY_H = LIBRARY_HEIGHT;
