#!/bin/sh
slangc kernel1D_ResolveDebug.slang -o kernel1D_ResolveDebug.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_FillGbuffer.slang -o kernel1D_FillGbuffer.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_FillGbufferWithTransparency.slang -o kernel1D_FillGbufferWithTransparency.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_ResolveOutline.slang -o kernel1D_ResolveOutline.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_AverageColor.slang -o kernel1D_AverageColor.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_Tonemap.slang -o kernel1D_Tonemap.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel2D_PackXY.slang -o kernel2D_PackXY.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_ResolveCommon.slang -o kernel1D_ResolveCommon.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_ResolveCommonWithTransparency.slang -o kernel1D_ResolveCommonWithTransparency.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
slangc kernel1D_ResolveCTF.slang -o kernel1D_ResolveCTF.comp.spv -I.. -I/home/frol/PROG/LiteRT_open/dependencies/1st-party/LiteMath -I/home/frol/PROG/kernel_slicer/TINYSTL 
