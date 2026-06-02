#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Declarations.h>

#include <RendererCore/RenderContext/RenderContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderViewContext, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezRenderViewContext::UpdateViewport() const
{
  ezRectFloat viewport = m_pViewData->m_ViewPortRect;
  auto& gc = m_pRenderContext->WriteGlobalConstants();
  gc.ViewportSize = ezVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);
  m_pRenderContext->GetCommandEncoder()->SetViewport(viewport);
}