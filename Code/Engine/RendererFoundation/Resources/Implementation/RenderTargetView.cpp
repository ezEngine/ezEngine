#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/RenderTargetView.h>


ezGALRenderTargetView::ezGALRenderTargetView(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& description)
  : ezGALObject(description)
  , m_pTexture(pTexture)
{
  EZ_ASSERT_DEV(m_pTexture != nullptr, "Texture must not be null");
}

ezGALRenderTargetView::~ezGALRenderTargetView() = default;
