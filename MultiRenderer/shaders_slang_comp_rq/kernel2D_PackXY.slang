//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common_slang_comp_rq_slang.h"

RWStructuredBuffer<uint> m_packedXY; // 
RWStructuredBuffer<MultiRenderer_slang_comp_rq_UBO_Data> ubo;

void PackXY(uint tidX, uint tidY) ;



void PackXY(uint tidX, uint tidY) {
  const uint offset   = SuperBlockIndex2DOpt(tidX, tidY, ubo[0].m_packedXY_width);
  m_packedXY[offset] = ((tidY << 16) & 0xFFFF0000) | (tidX & 0x0000FFFF);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct KernelArgs
{
  uint h; 
  uint w; 
  uint iNumElementsZ; 
  uint tFlagsMask;    
};



[shader("compute")]
[numthreads(32, 8, 1)]
void main(uint3 a_globalTID: SV_DispatchThreadID, uint3 a_localTID : SV_GroupThreadID, uniform KernelArgs kgenArgs)
{
  bool runThisThread = true;
  const uint y = uint(a_globalTID[0]); 
  const uint x = uint(a_globalTID[1]); 
  if(y >= kgenArgs.h + 0 || x >= kgenArgs.w + 0)
    runThisThread = false;
    // KERNEL BODY:
  //
  if(runThisThread)
  {
  
      PackXY(x, y);
    
  }
}

