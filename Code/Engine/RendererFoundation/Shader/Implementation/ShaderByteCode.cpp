#include <PCH.h>

#include <RendererFoundation/Shader/ShaderByteCode.h>

ezGALShaderByteCode::ezGALShaderByteCode() {}

ezGALShaderByteCode::ezGALShaderByteCode(const ezArrayPtr<const ezUInt8>& pByteCode)
{
  CopyFrom(pByteCode);
}

void ezGALShaderByteCode::CopyFrom(const ezArrayPtr<const ezUInt8>& pByteCode)
{
  EZ_ASSERT_DEV(pByteCode.GetPtr() != nullptr && pByteCode.GetCount() != 0, "Byte code is invalid!");

  m_Source = pByteCode;
}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_ShaderByteCode);
