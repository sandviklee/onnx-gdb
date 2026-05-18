#pragma once
#include "node.h"
#include <memory>
#include <vector>

#ifndef MODEL_H
#define MODEL_H

class Model {
private:
public:
  std::vector<std::unique_ptr<Node>> nodes;
  std::vector<Node *> roots;
  std::vector<Node *> leafs;
};

#endif
