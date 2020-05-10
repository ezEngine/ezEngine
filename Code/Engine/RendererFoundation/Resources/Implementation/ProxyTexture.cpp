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


const ezGALResourceBase* ezGALProxyTexture::GetParentResource() const
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

ezResult ezGALProxyTexture::ReplaceExisitingNativeObject(void* pExisitingNativeObject)
{
  //TODO
  return EZ_FAILURE;
}

void ezGALProxyTexture::SetDebugNamePlatform(const char* szName) const {}


EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_ProxyTexture);

