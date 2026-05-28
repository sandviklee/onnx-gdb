#include "ir/operator.h"

namespace ir {

static AttributeValue av_int(int64_t v) {
  AttributeValue a;
  a.type = AttrType::INT;
  a.i = v;
  return a;
}
static AttributeValue av_float(float v) {
  AttributeValue a;
  a.type = AttrType::FLOAT;
  a.f = v;
  return a;
}
static AttributeValue av_ints(std::vector<int64_t> v) {
  AttributeValue a;
  a.type = AttrType::INTS;
  a.ints = std::move(v);
  return a;
}

const std::unordered_map<std::string, OperatorSpec> &operator_registry() {
  static const std::unordered_map<std::string, OperatorSpec> registry = {
      {"PortInput", {"PortInput", OperatorCategory::IO, 0, 1, {}}},
      {"PortOutput", {"PortOutput", OperatorCategory::IO, 1, 0, {}}},
      {"MatMul", {"MatMul", OperatorCategory::MATH, 2, 1, {}}},
      {"Add", {"Add", OperatorCategory::MATH, 2, 1, {}}},
      {"Sub", {"Sub", OperatorCategory::MATH, 2, 1, {}}},
      {"Mul", {"Mul", OperatorCategory::MATH, 2, 1, {}}},
      {"Div", {"Div", OperatorCategory::MATH, 2, 1, {}}},
      {"Pow", {"Pow", OperatorCategory::MATH, 2, 1, {}}},
      {"Gemm",
       {"Gemm",
        OperatorCategory::MATH,
        3,
        1,
        {{"alpha", av_float(1.0f)},
         {"beta", av_float(1.0f)},
         {"transA", av_int(0)},
         {"transB", av_int(0)}}}},
      {"Abs", {"Abs", OperatorCategory::MATH, 1, 1, {}}},
      {"Neg", {"Neg", OperatorCategory::MATH, 1, 1, {}}},
      {"Exp", {"Exp", OperatorCategory::MATH, 1, 1, {}}},
      {"Sqrt", {"Sqrt", OperatorCategory::MATH, 1, 1, {}}},
      {"Log", {"Log", OperatorCategory::MATH, 1, 1, {}}},
      {"Relu", {"Relu", OperatorCategory::ACTIVATION, 1, 1, {}}},
      {"Sigmoid", {"Sigmoid", OperatorCategory::ACTIVATION, 1, 1, {}}},
      {"Tanh", {"Tanh", OperatorCategory::ACTIVATION, 1, 1, {}}},
      {"LeakyRelu",
       {"LeakyRelu",
        OperatorCategory::ACTIVATION,
        1,
        1,
        {{"alpha", av_float(0.01f)}}}},
      {"Elu",
       {"Elu",
        OperatorCategory::ACTIVATION,
        1,
        1,
        {{"alpha", av_float(1.0f)}}}},
      {"Softmax",
       {"Softmax", OperatorCategory::ACTIVATION, 1, 1, {{"axis", av_int(-1)}}}},
      {"Conv",
       {"Conv",
        OperatorCategory::LAYER,
        2,
        1,
        {{"kernel_shape", av_ints({3, 3})},
         {"strides", av_ints({1, 1})},
         {"pads", av_ints({0, 0, 0, 0})},
         {"dilations", av_ints({1, 1})},
         {"group", av_int(1)}}}},
      {"MaxPool",
       {"MaxPool",
        OperatorCategory::LAYER,
        1,
        1,
        {{"kernel_shape", av_ints({2, 2})},
         {"strides", av_ints({2, 2})},
         {"pads", av_ints({0, 0, 0, 0})}}}},
      {"AveragePool",
       {"AveragePool",
        OperatorCategory::LAYER,
        1,
        1,
        {{"kernel_shape", av_ints({2, 2})},
         {"strides", av_ints({2, 2})},
         {"pads", av_ints({0, 0, 0, 0})}}}},
      {"GlobalAveragePool",
       {"GlobalAveragePool", OperatorCategory::LAYER, 1, 1, {}}},
      {"Flatten",
       {"Flatten", OperatorCategory::LAYER, 1, 1, {{"axis", av_int(1)}}}},
      {"Reshape", {"Reshape", OperatorCategory::LAYER, 2, 1, {}}},
      {"Transpose", {"Transpose", OperatorCategory::LAYER, 1, 1, {}}},
      {"Concat",
       {"Concat", OperatorCategory::LAYER, 2, 1, {{"axis", av_int(1)}}}},
  };
  return registry;
}

} // namespace ir
