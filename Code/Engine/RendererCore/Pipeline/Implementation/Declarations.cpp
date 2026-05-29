#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Declarations.h>

#include <RendererCore/RenderContext/RenderContext.h>

void ezRenderViewContext::UpdateViewport() const
{
  ezRectFloat viewport = m_pViewData->m_ViewPortRect;
  auto& gc = m_pRenderContext->WriteGlobalConstants();
  gc.ViewportSize = ezVec4(viewport.width, viewport.height, 1.0f / viewport.width, 1.0f / viewport.height);
  m_pRenderContext->GetCommandEncoder()->SetViewport(viewport);
}