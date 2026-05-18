#pragma once
#include <string>

namespace ir {

class IBackendPort {
public:
    virtual ~IBackendPort() = default;
    virtual void run_inference() = 0;
    virtual void run_debug_inference() = 0;
    virtual bool load_onnx_file(const std::string& path, std::string& error) = 0;
};

} // namespace ir
