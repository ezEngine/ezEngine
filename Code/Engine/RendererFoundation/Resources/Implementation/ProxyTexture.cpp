#include <RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ProxyTexture.h>

namespace
{
  ezGALTextureCreationDescription MakeProxyDesc(const ezGALTextureCreationDescription& parentDesc)
  {
    ezGALTextureCreationDescription desc = parentDesc;
    desc.m_Type = ezGALTextureType::Texture2DProxy;
    return desc;
  }
} // namespace

ezGALProxyTexture::ezGALProxyTexture(const ezGALTexture& parentTexture)
  : ezGALTexture(MakeProxyDesc(parentTexture.GetDescription()))
  , m_pParentTexture(&parentTexture)
{
}

ezGALProxyTexture::~ezGALProxyTexture() {}


const ezGALTexture* ezGALProxyTexture::GetParentTexture() const
{
  return m_pParentTexture;
}

ezResult ezGALProxyTexture::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  return EZ_SUCCESS;
}

ezResult ezGALProxyTexture::DeInitPlatform(ezGALDevice* pDevice)
{
  return EZ_SUCCESS;
}

void ezGALProxyTexture::SetDebugName(const char* szName) const {}
