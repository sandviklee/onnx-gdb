cmake_minimum_required(VERSION 3.15)
project(onnx-gdb LANGUAGES C CXX)

set(RAYLIB_VERSION 5.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LINUX_ONNX_RUNTIME_LIB_DIR "onnxruntime-linux-x64-1.24.4")

option(ENABLE_ONNXMLIR_BACKEND "Build the ONNX-MLIR backend" OFF)
if(NOT ONNXMLIR_ROOTDIR)
  if(APPLE)
    set(ONNXMLIR_ROOTDIR "~/onnx_mlir/")
  else()
    set(ONNXMLIR_ROOTDIR "/usr/local")
  endif()
endif()

if(WIN32)
  add_compile_options(/W4)
  add_compile_definitions(NOMINMAX)
else()
  add_compile_options(-Wall -Wextra)
endif()

include(fetchcontent)

if(NOT ONNXRUNTIME_ROOTDIR)
  if(APPLE)
    set(ONNXRUNTIME_ROOTDIR "/opt/homebrew/opt/onnxruntime")
  elseif(WIN32)
    set(ONNXRUNTIME_ROOTDIR "C:/Program Files/onnxruntime")
  else()
    set(ONNXRUNTIME_ROOTDIR "/usr/local/${LINUX_ONNX_RUNTIME_LIB_DIR}")
  endif()
endif()

fetchcontent_declare(
  onnx
  GIT_REPOSITORY https://github.com/onnx/onnx.git
  GIT_TAG v1.16.0
)
set(ONNX_ML ON CACHE BOOL "" FORCE)
set(BUILD_ONNX_PYTHON OFF CACHE BOOL "" FORCE)
fetchcontent_makeavailable(onnx)

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

add_library(ir
    src/ir/operator.cpp
    src/ir/node.cpp
    src/ir/graph.cpp
)
target_include_directories(ir PUBLIC include)
target_link_libraries(ir PUBLIC onnx)

add_library(backend src/backend/backend.cpp)
if(APPLE)
  target_include_directories(backend PUBLIC
    include
    "${ONNXRUNTIME_ROOTDIR}/include/onnxruntime"
    "${ONNXRUNTIME_ROOTDIR}/include/onnxruntime/core/session"
  )
else()
  target_include_directories(backend PUBLIC
    include
    "${ONNXRUNTIME_ROOTDIR}/include"
  )
endif()
target_link_libraries(backend PUBLIC onnx ir)

add_library(gdbui
    src/ui/toolbar.cpp
    src/ui/graph.cpp
    src/ui/block.cpp
    src/ui/library.cpp
)
target_include_directories(gdbui PUBLIC include)
target_link_libraries(gdbui PUBLIC raylib ir)

target_sources(gdbui PRIVATE ${CMAKE_BINARY_DIR}/raygui_impl.c)
target_include_directories(gdbui PRIVATE ${raygui_SOURCE_DIR}/src)

if(ENABLE_ONNXMLIR_BACKEND)
  if(NOT ONNX_MLIR_SRC)
    if(DEFINED ENV{ONNX_MLIR_SRC})
      set(ONNX_MLIR_SRC "$ENV{ONNX_MLIR_SRC}")
    else()
      set(ONNX_MLIR_SRC "/Users/sandviklee/onnx-mlir")
    endif()
  endif()
  set(ONNX_MLIR_LIB_DIR "${ONNX_MLIR_SRC}/build/Release/lib")
  set(ONNX_MLIR_INC_DIR "${ONNX_MLIR_SRC}/include")
  set(ONNX_MLIR_BIN "${ONNX_MLIR_SRC}/build/Release/bin/onnx-mlir")
  if(NOT EXISTS "${ONNX_MLIR_LIB_DIR}/libcruntime.a")
    message(FATAL_ERROR "libcruntime.a not found at ${ONNX_MLIR_LIB_DIR}; "
            "set -DONNX_MLIR_SRC=/path/to/onnx-mlir")
  endif()
  if(NOT EXISTS "${ONNX_MLIR_BIN}")
    message(FATAL_ERROR "onnx-mlir CLI not found at ${ONNX_MLIR_BIN}; "
            "set -DONNX_MLIR_SRC=/path/to/onnx-mlir")
  endif()
  message(STATUS "Using ONNX_MLIR_SRC=${ONNX_MLIR_SRC}")

  add_library(onnxmlir_backend src/backend/onnxmlir_backend.cpp)
  target_include_directories(onnxmlir_backend PUBLIC include "${ONNX_MLIR_INC_DIR}")
  target_link_libraries(onnxmlir_backend PUBLIC
    onnx ir "${ONNX_MLIR_LIB_DIR}/libcruntime.a" ${CMAKE_DL_LIBS})
  target_compile_definitions(onnxmlir_backend PRIVATE
    ROMLL_ONNXMLIR_BIN="${ONNX_MLIR_BIN}")
  target_compile_options(onnxmlir_backend PRIVATE -Wall -Wextra)
endif()

add_executable(${PROJECT_NAME} src/main.cpp src/ui/filedlg.cpp)
target_link_directories(${PROJECT_NAME} PRIVATE
  "${ONNXRUNTIME_ROOTDIR}/lib"
)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE onnxruntime backend gdbui)

if(ENABLE_ONNXMLIR_BACKEND)
  target_link_libraries(${PROJECT_NAME} PRIVATE onnxmlir_backend)
  target_compile_definitions(${PROJECT_NAME} PRIVATE ROMLL_ONNXMLIR_AVAILABLE)
endif()

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "16.0" CACHE STRING "" FORCE)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    "-framework IOKit"
    "-framework Cocoa"
    "-framework OpenGL"
  )
endif()

target_compile_options(ir      PRIVATE -Wall -Wextra)
target_compile_options(backend PRIVATE -Wall -Wextra)
target_compile_options(gdbui   PRIVATE -Wall -Wextra)
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
