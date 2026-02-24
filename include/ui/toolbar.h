#pragma once
#include "raylib.h"
#include <vector>

struct toolbar_button {
  Rectangle rect;
  const char *text;
};

void draw_toolbar(const std::vector<toolbar_button> buttons);
int handle_toolbar_click(const std::vector<toolbar_button> &buttons);
