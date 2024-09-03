#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Resources/ResourceView.h>


ezGALTextureResourceView::ezGALTextureResourceView(ezGALTexture* pResource, const ezGALTextureResourceViewCreationDescription& description)
  : ezGALObject(description)
  , m_pResource(pResource)
{
  EZ_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

ezGALTextureResourceView::~ezGALTextureResourceView() = default;

ezGALBufferResourceView::ezGALBufferResourceView(ezGALBuffer* pResource, const ezGALBufferResourceViewCreationDescription& description)
  : ezGALObject(description)
  , m_pResource(pResource)
{
  EZ_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

ezGALBufferResourceView::~ezGALBufferResourceView() = default;


