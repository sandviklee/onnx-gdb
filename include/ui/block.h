#pragma once
#include "ir/node.h"
#include "raylib.h"
#include "ui/config.h"
#include <vector>

namespace ui {

enum class PortKind { INPUT, OUTPUT };

struct InputFieldState {
    ir::Node* active_node = nullptr;
    int active_field = -1;
    char buffer[32] = {};
    int cursor = -1;
};

struct ShapePopupState {
    ir::Node* target = nullptr;
    int pending_rank = 1;
    int pending_dims[3] = {1, 1, 1};
    bool active = false;
};

Color operator_category_color(ir::OperatorCategory category);
int operator_category_order(ir::OperatorCategory category);

class Block {
public:
    ir::Node* node;
    Vector2 position;
    float width = BLOCK_W;
    float height;

    Block(ir::Node* node, const Vector2& position);

    float calculate_height() const;
    std::vector<Vector2> calculate_input_ports() const;
    std::vector<Vector2> calculate_output_ports() const;
    std::vector<Rectangle> calculate_field_rects() const;

    void draw(const InputFieldState& input_state) const;
    void update_height();
};

} // namespace ui
