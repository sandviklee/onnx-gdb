#pragma once
#include "ir/operator.h"
#include <cstdint>
#include <string>
#include <vector>

namespace ir {

class Node;

struct Edge {
    Node* node;
    size_t port_index;
};

struct LayoutHint {
    float x = 0.0f;
    float y = 0.0f;
};

class Node {
public:
    const OperatorSpec* spec;
    std::string label;

    std::vector<Edge> inputs;
    std::vector<Edge> outputs;

    std::vector<float> values;
    std::vector<int> shape_dims;
    bool has_results = false;

    std::vector<float> debug_output_values;
    std::vector<int64_t> debug_output_shape;
    bool has_debug_values = false;

    LayoutHint layout_hint;

    Node(const std::string& op_name, const std::string& label);
};

} // namespace ir
