#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Basics.h>

const ezUInt8 ezGALIndexType::Size[ezGALIndexType::ENUM_COUNT] =
{
  sizeof(ezInt16),  // UShort
  sizeof(ezInt32)   // UInt
};



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Basics);

