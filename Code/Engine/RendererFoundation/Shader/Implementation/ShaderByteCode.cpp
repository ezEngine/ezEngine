
#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>

ezGALShaderByteCode::ezGALShaderByteCode()
{
}

ezGALShaderByteCode::ezGALShaderByteCode(const ezArrayPtr<const ezUInt8>& pByteCode)
{
  CopyFrom(pByteCode);
}

ezGALShaderByteCode::ezGALShaderByteCode(const void* pSource, ezUInt32 uiSize)
{
  CopyFrom(ezArrayPtr<const ezUInt8>(static_cast<const ezUInt8*>(pSource), uiSize));
}

void ezGALShaderByteCode::CopyFrom(const ezArrayPtr<const ezUInt8>& pByteCode)
{
  EZ_ASSERT_DEV(pByteCode.GetPtr() != nullptr && pByteCode.GetCount() != 0, "Byte code is invalid!");

  m_Source.Clear();

  m_Source.PushBackRange(pByteCode);
}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_ShaderByteCode);

