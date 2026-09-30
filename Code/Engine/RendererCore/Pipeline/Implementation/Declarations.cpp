#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Declarations.h>

#include <RendererCore/RenderContext/RenderContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderViewContext, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezRenderViewContext::SetUnscaledViewport() const
{
  m_pRenderContext->SetViewport(m_pViewData->m_ViewPortRect);
}

void ezRenderViewContext::SetScaledViewport() const
{
  m_pRenderContext->SetViewport(m_pViewData->GetScaledViewport());
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Declarations);
