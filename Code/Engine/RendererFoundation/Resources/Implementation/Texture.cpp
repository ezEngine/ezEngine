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
