#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace ir {

struct TensorData {
  std::vector<float> values;
  std::vector<int64_t> shape;
};

class IBackendPort {
public:
  virtual ~IBackendPort() = default;
  virtual void run_inference() = 0;
  virtual void run_debug_inference() = 0;

  virtual std::string get_name() = 0;
  virtual TensorData run_mini_model(const std::string &bytes,
                                    const std::vector<TensorData> &inputs) = 0;
};

} // namespace ir
