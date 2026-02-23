cmake_minimum_required(VERSION 3.13)
project(blockly-onnx LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
  add_compile_options(/W4)
  add_compile_definitions(NOMINMAX)
else()
  add_compile_options(-Wall -Wextra)
endif()

# NOTE: This fetches the ONNX API
include(FetchContent)
FetchContent_Declare(
  onnx
  GIT_REPOSITORY https://github.com/onnx/onnx.git
  GIT_TAG v1.16.0
)
set(ONNX_ML ON CACHE BOOL "" FORCE)
set(BUILD_ONNX_PYTHON OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(onnx)

# NOTE: This includes the ONNX Runtime API through self install, read the README for more information
option(ONNXRUNTIME_ROOTDIR "onnxruntime root dir" "")
if(NOT ONNXRUNTIME_ROOTDIR)
  if(APPLE)
    set(ONNXRUNTIME_ROOTDIR "/opt/homebrew/opt/onnxruntime")
  elseif(WIN32)
    set(ONNXRUNTIME_ROOTDIR "C:/Program Files/onnxruntime")
  else()
    set(ONNXRUNTIME_ROOTDIR "/usr/local")
  endif()
endif()

add_library(boir src/boir/boir.cpp)
target_include_directories(boir PUBLIC include)

add_executable(blockly-onnx src/main.cpp)
target_include_directories(blockly-onnx PRIVATE
  "${ONNXRUNTIME_ROOTDIR}/include"
  "${ONNXRUNTIME_ROOTDIR}/include/onnxruntime/core/session"
)
target_link_directories(blockly-onnx PRIVATE
  "${ONNXRUNTIME_ROOTDIR}/lib"
)
target_link_libraries(blockly-onnx PRIVATE onnx onnxruntime boir)
