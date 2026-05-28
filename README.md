# ONNX Graphical DeBugger

### Abstract

ML (Machine Learning) has a high barrier to entry, not just because the
topic is complex, but because you end up navigating a mess of different
frameworks and tools. Since most of these also require programming
proficiency, they exclude those without software development experience,
even though the underlying ML concepts are often intuitive once
visualized. GUIs (Graphical User Interfaces) have been used for more
approachable ML model creation, but these are often dependent on other
feature complete ML frameworks such as Tensorflow. This coupling
makes it harder to focus on increasing efficiency, which is why with onnx-gdb, we look into the problem of extending the field of GUI based ML by
creating an efficient GUI based ML compiler, built with the ONNX (Open
Neural Network Exchange) instruction set and model. This makes a novel approach for creating ONNX models with Visual Programming bundled with both **debug** and **editing** features.

### Overview

ONNX-GDB is an all in one visual ONNX environment, with the goal of simplifying construction, modification, inference and debugging of ONNX models. It tries to contribute to fixing the fragmentation that plagues the Machine Learning ecosystem. ONNX-GDB is still under development, and most of the features are still in early stages. This means that there WILL be bugs.

Due to the hexagonal structure of the project, ONNX-GDB supports two multiple execution backends. The ONNX Runtime (ORT) and ONNX-MLIR. This is to make it more accessible to users less comfortable with setting up execution environments. This is especially prevalent for ONNX-MLIR, which requires more complex setup.

To change between execution backends, simply compile with the correct flags, and make sure to update the main.cpp and mlir src files accordingly.

### Requirements

To run this project, you will need to have the following dependencies installed:

- CMAKE 3.15 or higher
- Clang or GCC
- Raylib SDK
- ONNX SDK
- ONNX Runtime SDK
- LLVM Source (For ONNX-MLIR backend, optional)
- ONNX-MLIR (For ONNX_MLIR backend, optional)

### Installation

As stated in the Requirements section, we need the ONNX SDK and ONNX Runtime:

#### ONNX Runtime

##### MacOS through Homebrew

```bash
brew install onnxruntime
```

then double check the installed path:

```bash
brew --prefix onnxruntime
```

and update the CMake_Lists.txt accordingly.

##### Linux

As for Linux type operating systems, you can just install the binary directly from GitHub and update the CMake_Lists.txt accordingly. Follow a guide specific for that distro.

##### Another approach

it is also possible to configure so it is downloaded with CMAKE, but since I already had it installed as a binary, I used that instead.

To use CMAKE to install the runtime, paste this into the CMake_Lists.txt:

```CMake
FetchContent_Declare(
  onnxruntime
  GIT_REPOSITORY https://github.com/microsoft/onnxruntime.git
  GIT_TAG v1.23.2 # For current latest update, check the repository
)
FetchContent_MakeAvailable(onnxruntime)
```

Be sure to remove the already existing import.

#### ONNX & Raylib SDK

The ONNX & Raylib SDK is set to be installed with CMake_Lists, this is because it includes all the dependencies needed.
Less hassle...

#### ONNX-MLIR

Please read the official ONNX-MLIR documentation for installation instructions, as it is a bit more complex than the other dependencies.

<https://onnx.ai/onnx-mlir/BuildOnLinuxOSX.html>

### Build and Run

Build with CMAKE:

```bash
cd blockly-onnx
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5
# If you want to specify ONNX-MLIR backend (be sure to change main.cpp and mlir src accordingly):
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DENABLE_ONNXMLIR_BACKEND=ON -DONNX_MLIR_SRC="path-to-onnx-mlir-source"
cmake --build build

```

Execute:

```bash
cd build
./blockly-onnx
```
