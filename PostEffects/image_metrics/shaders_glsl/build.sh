#!/bin/sh
glslangValidator -V kernel1D_ArraySumm_Init.comp -o kernel1D_ArraySumm_Init.comp.spv -DGLSL -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
glslangValidator -V kernel1D_ArraySumm.comp -o kernel1D_ArraySumm.comp.spv -DGLSL -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
glslangValidator -V kernel1D_ArraySumm_Reduction.comp -o kernel1D_ArraySumm_Reduction.comp.spv -DGLSL -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
