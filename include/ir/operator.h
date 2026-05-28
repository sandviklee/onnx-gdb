#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ir {

enum class OperatorCategory { IO, ACTIVATION, MATH, LAYER };

enum class AttrType { INT, FLOAT, INTS, FLOATS, STRING };

struct AttributeValue {
  AttrType type = AttrType::INT;
  int64_t i = 0;
  float f = 0.0f;
  std::vector<int64_t> ints;
  std::vector<float> floats;
  std::string s;
};

struct AttributeSpec {
  std::string name;
  AttributeValue default_value;
};

struct OperatorSpec {
  std::string name;
  OperatorCategory category;
  size_t num_inputs;
  size_t num_outputs;
  std::vector<AttributeSpec> attributes;
};

const std::unordered_map<std::string, OperatorSpec> &operator_registry();

} // namespace ir
