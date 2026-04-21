//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_comp_rq_slang.h"

RWStructuredBuffer<float4> out_color; // 
RWStructuredBuffer<float4> m_colorBuffer; // 
RWStructuredBuffer<MultiRenderer_slang_comp_rq_UBO_Data> ubo;

float4 AverageColor(uint tidX, uint a_sample_count) ;



float4 AverageColor(uint tidX, uint a_sample_count) {
  return m_colorBuffer[tidX] / float(a_sample_count);
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
  out_color[tidX] = AverageColor(tidX, kgenArgs.sample_count);
  }
}

