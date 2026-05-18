#include "ir/node.h"

namespace ir {

Node::Node(const std::string &op_name, const std::string &label)
    : spec(&operator_registry().at(op_name)), label(label) {}

} // namespace ir
