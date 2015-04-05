#include <RendererCore/PCH.h>
#include <RendererCore/RendererCore.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>

GlobalConstants ezRendererCore::s_GlobalConstants;
bool ezRendererCore::s_bGlobalConstantsModified = true;
ezConstantBufferResourceHandle ezRendererCore::s_hGlobalConstantBuffer;

void ezRendererCore::UploadGlobalConstants()
{
  if (!s_bGlobalConstantsModified)
    return;

  s_bGlobalConstantsModified = false;

  if (s_hGlobalConstantBuffer.IsValid())
  {
    GlobalConstants* pBuffer = BeginModifyConstantBuffer<GlobalConstants>(s_hGlobalConstantBuffer);

    ezMemoryUtils::Copy(pBuffer, &s_GlobalConstants, 1);

    EndModifyConstantBuffer();
  }
  else
  {
    ezConstantBufferResourceDescriptor<GlobalConstants> rd;
    rd.m_Data = s_GlobalConstants;
    s_hGlobalConstantBuffer = ezResourceManager::CreateResource<ezConstantBufferResource>("ezGlobalConstantBuffer", rd);
  }

  BindConstantBuffer("GlobalConstants", s_hGlobalConstantBuffer);
}


