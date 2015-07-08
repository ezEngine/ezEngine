#include <RendererCore/PCH.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>

GlobalConstants ezRenderContext::s_GlobalConstants;
bool ezRenderContext::s_bGlobalConstantsModified = true;
ezConstantBufferResourceHandle ezRenderContext::s_hGlobalConstantBuffer;

void ezRenderContext::UploadGlobalConstants()
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
    s_hGlobalConstantBuffer = ezResourceManager::CreateResource<ezConstantBufferResource>("ezGlobalConstantBuffer", rd, "ezGlobalConstantBuffer");
  }

  BindConstantBuffer("GlobalConstants", s_hGlobalConstantBuffer);
}


