# main options - what parts of the project to build
option(BUILD_DEMO_APP "Build Demo App" ON)
option(BUILD_RENDER_CLI "Build Render CLI application" ON)
option(BUILD_EXTENDED_DEMO_APP "Build Extended Demo App" ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# what additional features and libraries to use
option(USE_HYDRA "Use Hydra library and renderer" OFF)
option(USE_VULKAN "include GPU support" ON)
option(USE_GPU_RQ "include RTX support using Ray Query in compute shader" OFF)

# misc options
option(SPARSE_DR_QUANTIZE_GRAD "Quantize gradients" OFF)
option(USE_OPENVDB "Enable OpenVDB" OFF)
option(USE_KTX "Use libktx for image compression" OFF)

option(USE_LIBIGL "Enable LibIGL for chamfer and hausdorff distances" OFF)

# leave defaults unless you know what you're doing
option(BUILD_EXE_IN_ROOT "Executables are put in project root directory instead of default path" ON)
option(USE_OMP "Enable OpenMP" ON)
option(USE_FAST_MATH "Enable ffast-math" ON)
option(USE_MARCH_NATIVE "Enable -march=native" ON)
option(USE_EXTRA_WARNINGS "Enable some extra warnings for LiteRT project" OFF)
option(LITERT_WARNINGS_AS_ERRORS "Enable warnings as errors for LiteRT project" ON)

set(WEBGPU_BACKEND "DAWN" CACHE STRING "What webput backend to use: WGPU or DAWN")

# use webgpu backend
option(USE_WEBGPU "Enable WebGPU backend" OFF)
