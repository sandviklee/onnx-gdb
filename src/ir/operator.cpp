#include "ir/operator.h"

namespace ir {

const std::unordered_map<std::string, OperatorSpec> &operator_registry() {
  static const std::unordered_map<std::string, OperatorSpec> registry = {
      {"PortInput", {"PortInput", OperatorCategory::IO, 0, 1}},
      {"PortOutput", {"PortOutput", OperatorCategory::IO, 1, 0}},
      {"MatMul", {"MatMul", OperatorCategory::MATH, 2, 1}},
      {"Add", {"Add", OperatorCategory::MATH, 2, 1}},
      {"Sub", {"Sub", OperatorCategory::MATH, 2, 1}},
      {"Mul", {"Mul", OperatorCategory::MATH, 2, 1}},
      {"Div", {"Div", OperatorCategory::MATH, 2, 1}},
      {"Pow", {"Pow", OperatorCategory::MATH, 2, 1}},
      {"Gemm", {"Gemm", OperatorCategory::MATH, 3, 1}},
      {"Abs", {"Abs", OperatorCategory::MATH, 1, 1}},
      {"Neg", {"Neg", OperatorCategory::MATH, 1, 1}},
      {"Exp", {"Exp", OperatorCategory::MATH, 1, 1}},
      {"Sqrt", {"Sqrt", OperatorCategory::MATH, 1, 1}},
      {"Log", {"Log", OperatorCategory::MATH, 1, 1}},
      {"Relu", {"Relu", OperatorCategory::ACTIVATION, 1, 1}},
      {"Sigmoid", {"Sigmoid", OperatorCategory::ACTIVATION, 1, 1}},
      {"Tanh", {"Tanh", OperatorCategory::ACTIVATION, 1, 1}},
      {"LeakyRelu", {"LeakyRelu", OperatorCategory::ACTIVATION, 1, 1}},
      {"Elu", {"Elu", OperatorCategory::ACTIVATION, 1, 1}},
      {"Softmax", {"Softmax", OperatorCategory::ACTIVATION, 1, 1}},
      {"Conv", {"Conv", OperatorCategory::LAYER, 2, 1}},
      {"MaxPool", {"MaxPool", OperatorCategory::LAYER, 1, 1}},
      {"AveragePool", {"AveragePool", OperatorCategory::LAYER, 1, 1}},
      {"GlobalAveragePool",
       {"GlobalAveragePool", OperatorCategory::LAYER, 1, 1}},
      {"BatchNormalization",
       {"BatchNormalization", OperatorCategory::LAYER, 1, 1}},
      {"Dropout", {"Dropout", OperatorCategory::LAYER, 1, 1}},
      {"Flatten", {"Flatten", OperatorCategory::LAYER, 1, 1}},
      {"Reshape", {"Reshape", OperatorCategory::LAYER, 2, 1}},
      {"Transpose", {"Transpose", OperatorCategory::LAYER, 1, 1}},
      {"Concat", {"Concat", OperatorCategory::LAYER, 2, 1}},
  };
  return registry;
}

} // namespace ir
