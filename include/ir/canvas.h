#pragma once
#include "ir/model.h"
#include "ir/node.h"
#include <memory>
#include <vector>

#ifndef CANVAS_H
#define CANVAS_H

class Canvas {
private:
public:
  Model *model;
  std::vector<std::unique_ptr<Node>> orphans;
};

#endif
