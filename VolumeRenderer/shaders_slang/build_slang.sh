#!/bin/sh
slangc kernel1D_RaymarchGrid_Bresenham.slang -o kernel1D_RaymarchGrid_Bresenham.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_RaymarchGrid_DDA.slang -o kernel1D_RaymarchGrid_DDA.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_RaymarchGrid_DDA_Branchless.slang -o kernel1D_RaymarchGrid_DDA_Branchless.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_AverageColor.slang -o kernel1D_AverageColor.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_RaymarchSCom4D.slang -o kernel1D_RaymarchSCom4D.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_RaytraceGrid_SS.slang -o kernel1D_RaytraceGrid_SS.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_RaymarchSCom.slang -o kernel1D_RaymarchSCom.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_Tonemap.slang -o kernel1D_Tonemap.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_RaymarchDAG.slang -o kernel1D_RaymarchDAG.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel2D_PackXY.slang -o kernel2D_PackXY.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_RaymarchDAG4D.slang -o kernel1D_RaymarchDAG4D.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
