cmake_minimum_required(VERSION 3.15)
project(onnx-gdb LANGUAGES C CXX)

set(RAYLIB_VERSION 5.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LINUX_ONNX_RUNTIME_LIB_DIR "onnxruntime-linux-x64-1.24.4")

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

add_executable(${PROJECT_NAME} src/main.cpp src/ui/filedlg.cpp)
target_link_directories(${PROJECT_NAME} PRIVATE
  "${ONNXRUNTIME_ROOTDIR}/lib"
)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE onnxruntime backend gdbui)

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
