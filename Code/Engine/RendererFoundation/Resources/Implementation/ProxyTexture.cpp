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

ezGALProxyTexture::ezGALProxyTexture(ezGALTextureHandle hParentTexture, const ezGALTexture& parentTexture, ezUInt16 uiSlice)
  : ezGALTexture(MakeProxyDesc(parentTexture.GetDescription()))
  , m_hParentTexture(hParentTexture)
  , m_pParentTexture(&parentTexture)
  , m_uiSlice(uiSlice)
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
