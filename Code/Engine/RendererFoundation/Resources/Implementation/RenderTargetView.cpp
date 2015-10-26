
#include <RendererFoundation/PCH.h>
#include <RendererFoundation/Resources/RenderTargetView.h>


ezGALRenderTargetView::ezGALRenderTargetView(const ezGALRenderTargetViewCreationDescription& Description)
  : ezGALObject(Description)
{
}

ezGALRenderTargetView::~ezGALRenderTargetView()
{
}



EZ_STATICLINK_FILE(RendererFoundation, RendererFoundation_Resources_Implementation_RenderTargetView);

