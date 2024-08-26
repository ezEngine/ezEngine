#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/UnorderedAccesView.h>

ezGALTextureUnorderedAccessView::ezGALTextureUnorderedAccessView(ezGALTexture* pResource, const ezGALTextureUnorderedAccessViewCreationDescription& description)
  : ezGALObject(description)
  , m_pResource(pResource)
{
  EZ_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

ezGALTextureUnorderedAccessView::~ezGALTextureUnorderedAccessView() = default;

ezGALBufferUnorderedAccessView::ezGALBufferUnorderedAccessView(ezGALBuffer* pResource, const ezGALBufferUnorderedAccessViewCreationDescription& description)
  : ezGALObject(description)
  , m_pResource(pResource)
{
  EZ_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

ezGALBufferUnorderedAccessView::~ezGALBufferUnorderedAccessView() = default;
