
#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Resources/RenderTargetView.h>


ezGALRenderTargetView::ezGALRenderTargetView(ezGALResourceBase* pResource, const ezGALRenderTargetViewCreationDescription& description)
  : ezGALObject(description)
  , m_pResource(pResource)
{
  EZ_ASSERT_DEV(m_pResource != nullptr, "Resource must not be null");
}

ezGALRenderTargetView::~ezGALRenderTargetView()
{
}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_RenderTargetView);

