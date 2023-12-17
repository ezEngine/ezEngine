#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>

const ezUInt8 ezGALIndexType::s_Size[ezGALIndexType::ENUM_COUNT] = {
  0,               // None
  sizeof(ezInt16), // UShort
  sizeof(ezInt32)  // UInt
};

const char* ezGALShaderStage::Names[ENUM_COUNT] = {
  "VertexShader",
  "HullShader",
  "DomainShader",
  "GeometryShader",
  "PixelShader",
  "ComputeShader",
};


