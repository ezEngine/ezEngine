#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Basics.h>

const ezUInt8 ezGALIndexType::Size[ezGALIndexType::ENUM_COUNT] =
{
  sizeof(ezInt16),  // UShort
  sizeof(ezInt32)   // UInt
};

const char* ezGALShaderStage::Names[ENUM_COUNT] =
{
  "VertexShader",
  "HullShader",
  "DomainShader",
  "GeometryShader",
  "PixelShader",
  "ComputeShader",
};

EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Basics);

