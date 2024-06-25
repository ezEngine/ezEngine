#include <RendererFoundation/RendererFoundationPCH.h>

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

ezGALProxyTexture::~ezGALProxyTexture() = default;


const ezGALResourceBase* ezGALProxyTexture::GetParentResource() const
{
  return m_pParentTexture;
}

ezResult ezGALProxyTexture::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  EZ_IGNORE_UNUSED(pDevice);
  EZ_IGNORE_UNUSED(pInitialData);

  return EZ_SUCCESS;
}

ezResult ezGALProxyTexture::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_IGNORE_UNUSED(pDevice);

  return EZ_SUCCESS;
}

void ezGALProxyTexture::SetDebugNamePlatform(const char* szName) const
{
  EZ_IGNORE_UNUSED(szName);
}
