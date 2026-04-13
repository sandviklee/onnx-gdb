cmake_minimum_required(VERSION 3.15)
project(raylib_onnx_machine_learning_language LANGUAGES C CXX)

set(RAYLIB_VERSION 5.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LINUX_ONNX_RUNTIME_LIB_DIR "onnxruntime-linux-x64-1.24.4") # This needs to be chnaged based on ONNX version and linux platform.

find_package(MLIR REQUIRED CONFIG)
include_directories(${MLIR_INCLUDE_DIRS})
list(APPEND CMAKE_MODULE_PATH "${MLIR_CMAKE_DIR}")
include(AddLLVM)
include(TableGen)
include(AddMLIR)

if(WIN32)
  add_compile_options(/W4)
  add_compile_definitions(NOMINMAX)
else()
  add_compile_options(-Wall -Wextra)
endif()

include(fetchcontent)

# NOTE: This includes the ONNX Runtime API through self install, read the README for more information
if(NOT ONNXRUNTIME_ROOTDIR)
  if(APPLE)
    set(ONNXRUNTIME_ROOTDIR "/opt/homebrew/opt/onnxruntime")
  elseif(WIN32)
    set(ONNXRUNTIME_ROOTDIR "C:/Program Files/onnxruntime")
  else()
    set(ONNXRUNTIME_ROOTDIR "/usr/local/${LINUX_ONNX_RUNTIME_LIB_DIR}")
  endif()
endif()

# NOTE: This fetches the ONNX API
fetchcontent_declare(
  onnx
  GIT_REPOSITORY https://github.com/onnx/onnx.git
  GIT_TAG v1.16.0
)
set(ONNX_ML ON CACHE BOOL "" FORCE)
set(BUILD_ONNX_PYTHON OFF CACHE BOOL "" FORCE)
fetchcontent_makeavailable(onnx)

# add_subdirectory(src/romll/Dialect/IR)
# add_subdirectory(src/romll/Dialect/Transforms)

find_package(raylib ${RAYLIB_VERSION} QUIET)
if(NOT raylib_FOUND)
  fetchcontent_declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  fetchcontent_getproperties(raylib)
  if(NOT raylib_POPULATED)
    set(FETCHCONTENT_QUIET NO)
    fetchcontent_makeavailable(raylib)
  endif()
endif()

fetchcontent_declare(
  raygui
  GIT_REPOSITORY https://github.com/raysan5/raygui.git
  GIT_TAG master
)
fetchcontent_makeavailable(raygui)

file(WRITE ${CMAKE_BINARY_DIR}/raygui_impl.c
  "#define RAYGUI_IMPLEMENTATION\n#include \"raygui.h\"\n"
)

add_library(romll src/romll/romll.cpp)
if(APPLE)
  target_include_directories(romll PUBLIC
  include
  "${ONNXRUNTIME_ROOTDIR}/include/onnxruntime"
  "${ONNXRUNTIME_ROOTDIR}/include/onnxruntime/core/session"
)
else()
  target_include_directories(romll PUBLIC
  include
  "${ONNXRUNTIME_ROOTDIR}/include"
)
endif()
target_link_libraries(romll PUBLIC onnx romllui)

add_library(romllui src/ui/toolbar.cpp src/ui/graph.cpp src/ui/block.cpp)
target_include_directories(romllui PUBLIC include)
target_link_libraries(romllui PUBLIC raylib)

target_sources(romllui PRIVATE ${CMAKE_BINARY_DIR}/raygui_impl.c)
target_include_directories(romllui PRIVATE ${raygui_SOURCE_DIR}/src)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_directories(${PROJECT_NAME} PRIVATE
  "${ONNXRUNTIME_ROOTDIR}/lib"
)
target_link_libraries(${PROJECT_NAME} PRIVATE onnxruntime romll romllui)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "16.0" CACHE STRING "" FORCE)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    "-framework IOKit"
    "-framework Cocoa"
    "-framework OpenGL"
  )
endif()

target_compile_options(romll PRIVATE -Wall -Wextra)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
