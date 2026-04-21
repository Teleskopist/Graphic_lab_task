#!/bin/sh
glslangValidator -V kernel2D_filter.comp -o kernel2D_filter.comp.spv -DGLSL -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
glslangValidator -V kernel2D_filterFloat.comp -o kernel2D_filterFloat.comp.spv -DGLSL -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
