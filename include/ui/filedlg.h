#pragma once
#include <string>

// Opens a native file dialog filtered to .onnx files.
// Returns the selected path, or "" if cancelled.
std::string open_onnx_file_dialog();
