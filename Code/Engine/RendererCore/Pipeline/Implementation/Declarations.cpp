#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Declarations.h>

#include <RendererCore/RenderContext/RenderContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderViewContext, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

void ezRenderViewContext::SetViewportIfSupported() const
{
  if (m_pViewData->m_fRenderScale == 1.0f)
  {
    m_pRenderContext->SetViewport(m_pViewData->m_ViewPortRect);
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Declarations);
