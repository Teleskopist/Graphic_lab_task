//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_comp_rq_slang.h"

RWStructuredBuffer<uint> out_color; // 
RWStructuredBuffer<float4> m_colorBuffer; // 
RWStructuredBuffer<MultiRenderer_slang_comp_rq_UBO_Data> ubo;

uint Tonemap(uint tidX, uint a_sample_count) ;



uint Tonemap(uint tidX, uint a_sample_count) {
  float4 color_f = m_colorBuffer[tidX] / float(a_sample_count);
  uint4 col = uint4(255 * clamp(color_f, float4(0,0,0,0), float4(1,1,1,1)));
  uint color_rgba8 = (col.w<<24) | (col.z<<16) | (col.y<<8) | col.x;
  return color_rgba8;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct KernelArgs
{
  uint sample_count;
  uint count; 
  uint iNumElementsY; 
  uint iNumElementsZ; 
  uint tFlagsMask;    
};



[shader("compute")]
[numthreads(256, 1, 1)]
void main(uint3 a_globalTID: SV_DispatchThreadID, uint3 a_localTID : SV_GroupThreadID, uniform KernelArgs kgenArgs)
{
  bool runThisThread = true;
  const uint tidX = uint(a_globalTID[0]); 
  if(tidX >= kgenArgs.count + 0)
    runThisThread = false;
    // KERNEL BODY:
  //
  if(runThisThread)
  {
  out_color[tidX] = Tonemap(tidX, kgenArgs.sample_count);
  }
}

