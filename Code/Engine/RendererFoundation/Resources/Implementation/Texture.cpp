#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/Texture.h>

ezGALTexture::ezGALTexture(const ezGALTextureCreationDescription& Description)
  : ezGALResource(Description)
{
}

ezGALTexture::~ezGALTexture()
{
  EZ_ASSERT_DEV(m_hDefaultResourceView.IsInvalidated(), "");
  EZ_ASSERT_DEV(m_hDefaultRenderTargetView.IsInvalidated(), "");

  EZ_ASSERT_DEV(m_ResourceViews.IsEmpty(), "Dangling resource views");
  EZ_ASSERT_DEV(m_RenderTargetViews.IsEmpty(), "Dangling render target views");
  EZ_ASSERT_DEV(m_UnorderedAccessViews.IsEmpty(), "Dangling unordered access views");
}

ezVec3U32 ezGALTexture::GetMipMapSize(ezUInt32 uiMipLevel) const
{
  ezVec3U32 size = {m_Description.m_uiWidth, m_Description.m_uiHeight, m_Description.m_uiDepth};
  size.x = ezMath::Max(1u, size.x >> uiMipLevel);
  size.y = ezMath::Max(1u, size.y >> uiMipLevel);
  size.z = ezMath::Max(1u, size.z >> uiMipLevel);
  return size;
}
