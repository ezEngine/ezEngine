#include <RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ProxyTexture.h>

namespace 
{
  ezGALTextureCreationDescription MakeProxyDesc(const ezGALTextureCreationDescription& parentDesc)
  {
    ezGALTextureCreationDescription desc = parentDesc;
    desc.m_Type = ezGALTextureType::Proxy;
    return desc;
  }
}

ezGALProxyTexture::ezGALProxyTexture(const ezGALTexture& parentTexture)
  : ezGALTexture(MakeProxyDesc(parentTexture.GetDescription()))
{
}

ezGALProxyTexture::~ezGALProxyTexture() {}

ezResult ezGALProxyTexture::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  return EZ_SUCCESS;
}

ezResult ezGALProxyTexture::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

void ezGALProxyTexture::SetDebugName(const char* szName) const {}
