# LiteRT

LiteRT is main repository of graphics group if the GML lab.
In consists of main (LiteRT) project and several others:
* [HydraCore3](HydraCore3/README.md)
* inverse CAD
* [Intiled GUI App](LiteApp/intiled_gui/README.md)
* [Soft RT](SoftRT/README.md)
* [LiteRT benchmark](benchmark/README.md)
* [Spectral](SpectralConverter/README.md)

## TLDR

    cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release -DUSE_VULKAN=ON -DUSE_HYDRA=ON && cmake --build build_release --target slice_all -j32 && cmake --build build_release -j32

## Build (Linux)
Tested on the following distros:
* Ubuntu 20 (with cmake 4.x from snap only)
* Ubuntu 22
* Ubuntu 24
* Mint ???

Install git, g++ and CMake if you dont have them:

    sudo apt install git g++ cmake

Clone this repo with all its submodules:

    git clone git@vg-code.gml-team.ru:graphics1/LiteRT.git
    cd LiteRT
    git submodule update --init --recursive

### CPU only
Many, but not all, parts of this project can be built without GPU support

    cmake -S . -B build_debug -DCMAKE_BUILD_TYPE=Debug -DUSE_VULKAN=OFF -DUSE_GPU_RQ=OFF 
    cmake --build build_debug -j16

    cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release -DUSE_VULKAN=OFF -DUSE_GPU_RQ=OFF 
    cmake --build build_release -j16


### GPU
GPU part of LiteRT uses Vulkan and relies on kernel slicer for code generation. GPU must support Vulkan (might not work on
NVidia Teslas and similar server GPUs). LiteRT runs almost everywhere, Hydra requires RTX support.

Install GLFW and glsl tools

    sudo apt install libglfw3-dev
    sudo apt install glslang-tools

Download and build kernel slicer (https://github.com/Ray-Tracing-Systems/kernel_slicer) somewhere outside LiteRT folder (e.g. *~/kernel_slicer*)

1) Configure CMake with your slicer paths (both folder) and executable:

        cmake -S . -B build_release -DUSE_VULKAN=ON -DCMAKE_BUILD_TYPE=Release -DKSLICER_PATH="~/kernel_slicer/cmake-build-release/kslicer" -DKSLICER_DIR="~/kernel_slicer/"

    The defaults are ~/kernel_slicer/ ~/kernel_slicer/kslicer
    
    There are many options for LiteRT compilation, see cmake/litert_project_options.cmake
2) Generate code with slicer

        cmake --build build_release --target slice_all -j16

    During the development, you can set specific targets to use slicer for a specific class (the one that was changed), e.g.

    cmake --build build_release --target slice_SparseDR_glsl

    See list of all slicer targets in cmake output

3) Compile

        cmake --build build_release -j16

Generate GPU-related code and shaders (use your path to slicer folder and executable):

    bash slicer_execute.sh ~/kernel_slicer/ ~/kernel_slicer/kslicer 
    run with "./webgpu_engine scenes/as1-oc-214.obj"

### Compute/RTX

For older and integrated GPUs:

    cmake -S . -B build_debug -DCMAKE_BUILD_TYPE=Debug -DUSE_VULKAN=ON -DUSE_GPU_RQ=OFF 

    cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release -DUSE_VULKAN=ON -DUSE_GPU_RQ=OFF 

For GPU with RTX support (tested only on Nvidia RTX GPUs):

    cmake -S . -B build_debug -DCMAKE_BUILD_TYPE=Debug -DUSE_VULKAN=ON -DUSE_GPU_RQ=ON 

    cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release -DUSE_VULKAN=ON -DUSE_GPU_RQ=ON 

Howto point kslicer path: -DKSLICER_PATH="kernel_slicer/cmake-build-release/kslicer" -DKSLICER_DIR="kernel_slicer"

### Build (GPU/WebGPU)
#### Native
    cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release -DUSE_VULKAN=OFF -DUSE_GPU_RQ=OFF -DUSE_WEBGPU=ON -DWEBGPU_BUILD_FROM_SOURCE=OFF -DWEBGPU_BACKEND=WGPU

Dawn

    cmake -S . -B build_release -DCMAKE_BUILD_TYPE=Release -DUSE_VULKAN=OFF -DUSE_GPU_RQ=OFF -DUSE_WEBGPU=ON -DWEBGPU_BUILD_FROM_SOURCE=OFF -DWEBGPU_BACKEND=DAWN

## Build (Windows)

Build with CMake is mostly the similar, Visual Studio build is not supported
TODO

## Launch

### Demo application (only with GPU support)

  Launch demo application
  Shaders will be compiled on the first launch, it can take a few minutes on weaker laptops and PCs

    ./engine

  Launch demo application and load scene

    ./engine -scene ./scenes/01_simple_scenes/instanced_objects.xml

  Launch on CPU with small window

    ./engine --CPU -w 640 -h 480 -scene ./scenes/01_simple_scenes/instanced_objects.xml

  Launch with specific renderer

    ./engine -renderer MULTI ./scenes/01_simple_scenes/instanced_objects.xml

### Offline renderer

  Render image, scene and settings taken from .blk config

    ./render_app config/render_paper_test.blk

### Tests

  Launch all tests

    ./testrt run

  Launch tests matching regular expression

    ./testrt run -r .*SDF.*

  Launch one test with specific name

    ./testrt exec AllTypesSanityCheck
  
### Benchmark 

  Launch benchmark

    ./benchmark_app <args>

