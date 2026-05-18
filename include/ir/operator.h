#pragma once
#include <string>
#include <unordered_map>

namespace ir {

enum class OperatorCategory { IO, ACTIVATION, MATH, LAYER };

struct OperatorSpec {
    std::string name;
    OperatorCategory category;
    size_t num_inputs;
    size_t num_outputs;
};

const std::unordered_map<std::string, OperatorSpec>& operator_registry();

} // namespace ir
