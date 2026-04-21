#!/bin/sh
slangc kernel2D_PackXY.slang -o kernel2D_PackXY.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_AverageColor.slang -o kernel1D_AverageColor.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_IntersectSVOFast.slang -o kernel1D_IntersectSVOFast.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_IntersectDAG.slang -o kernel1D_IntersectDAG.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_Tonemap.slang -o kernel1D_Tonemap.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_IntersectSCom.slang -o kernel1D_IntersectSCom.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_IntersectSVO.slang -o kernel1D_IntersectSVO.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
