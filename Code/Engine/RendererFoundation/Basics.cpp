#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Basics.h>

const ezUInt8 ezGALIndexType::Size[ezGALIndexType::Enum::ENUM_COUNT] =
{
  sizeof(ezInt16),  // UShort
  sizeof(ezInt32)   // UInt
};