cmake_minimum_required(VERSION 3.15)
project(raylib_onnx_machine_learning_language LANGUAGES CXX)

set(RAYLIB_VERSION 5.5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(LOCAL_ONNX_RUNTIME_LIB_DIR "onnxruntime-linux-x64-1.24.4") # This needs to be chnaged based on ONNX version and linux platform.:

if(WIN32)
  add_compile_options(/W4)
  add_compile_definitions(NOMINMAX)
else()
  add_compile_options(-Wall -Wextra)
endif()

include(FetchContent)

# NOTE: This includes the ONNX Runtime API through self install, read the README for more information
if(NOT ONNXRUNTIME_ROOTDIR)
  if(APPLE)
    set(ONNXRUNTIME_ROOTDIR "/opt/homebrew/opt/onnxruntime")
  elseif(WIN32)
    set(ONNXRUNTIME_ROOTDIR "C:/Program Files/onnxruntime")
  else()
    set(ONNXRUNTIME_ROOTDIR "/usr/local/${LOCAL_ONNX_RUNTIME_LIB_DIR}")
  endif()
endif()

# NOTE: This fetches the ONNX API
FetchContent_Declare(
  onnx
  GIT_REPOSITORY https://github.com/onnx/onnx.git
  GIT_TAG v1.16.0
)
set(ONNX_ML ON CACHE BOOL "" FORCE)
set(BUILD_ONNX_PYTHON OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(onnx)

find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
  endif()
endif()

add_library(romll src/romll/romll.cpp)
if (APPLE)
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
