
#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Shader/ShaderByteCode.h>

ezGALShaderByteCode::ezGALShaderByteCode()
  : m_pSource()
{
}

ezGALShaderByteCode::ezGALShaderByteCode(const ezArrayPtr<ezUInt8>& pByteCode)
  : m_pSource()
{
  CopyFrom(pByteCode);
}

ezGALShaderByteCode::ezGALShaderByteCode(void* pSource, ezUInt32 uiSize)
  : m_pSource()
{
  CopyFrom(ezArrayPtr<ezUInt8>(static_cast<ezUInt8*>(pSource), uiSize));
}

ezGALShaderByteCode::~ezGALShaderByteCode()
{
  Free();
}

void ezGALShaderByteCode::CopyFrom(const ezArrayPtr<ezUInt8>& pByteCode)
{
  EZ_ASSERT(pByteCode.GetPtr() != nullptr && pByteCode.GetCount() != 0, "Byte code is invalid!");

  Free();

  m_pSource = EZ_DEFAULT_NEW_ARRAY(ezUInt8, pByteCode.GetCount());

  ezMemoryUtils::Copy(m_pSource.GetPtr(), pByteCode.GetPtr(), pByteCode.GetCount());
}

void ezGALShaderByteCode::Free()
{
  if(m_pSource.GetPtr() != nullptr)
  {
    EZ_DEFAULT_DELETE_ARRAY(m_pSource);
  }

  m_pSource.Reset();
}