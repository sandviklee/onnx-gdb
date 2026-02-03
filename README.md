# blockly-onnx

ML (Machine Learning) has a high barrier to entry, not just because the
topic is complex, but because you end up navigating a mess of different
frameworks and tools. Since most of these also require programming
proficiency, they exclude those without software development experience,
even though the underlying ML concepts are often intuitive once
visualized. GUIs (Graphical User Interfaces) have been used for more
approachable ML model creation, but these are often dependent on other
feature complete ML frameworks such as Tensorflow. This coupling
makes it harder to focus on increasing efficiency, which is why in this
thesis, we look into the problem of extending the field of GUI based ML by
creating an efficient GUI based ML compiler, built with the ONNX (Open
Neural Network Exchange) instruction set. We will investigate
optimizations for improving interface accessibility (model creation) and
enhancing compilation performance through the usage of tools like ONNX-
MLIR. We validate our approach against a test suite of ML models,
evaluating both correctness and runtime performance.

### Requirements

To run this project, you will need to have the following installed:

- CMAKE 3.13 or higher
- Clang or GCC
- ONNX SDK (Comes with the CMAKE)
- ONNX Runtime

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

#### ONNX SDK

The ONNX SDK is set to be installed with CMake_Lists, this is because it includes all the dependencies needed.
Less hassle...

### Build and Run

Build with CMAKE:

```bash
cd blockly-onnx
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 # This is because the ONNX SDK needs to be installed
cmake --build build
```

Execute:

```bash
cd build
./blockly-onnx
```
