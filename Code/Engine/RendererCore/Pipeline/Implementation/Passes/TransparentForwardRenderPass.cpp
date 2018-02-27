#include <PCH.h>
#include <RendererCore/Pipeline/Passes/TransparentForwardRenderPass.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTransparentForwardRenderPass, 1, ezRTTIDefaultAllocator<ezTransparentForwardRenderPass>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTransparentForwardRenderPass::ezTransparentForwardRenderPass(const char* szName)
  : ezForwardRenderPass(szName)
{
}

ezTransparentForwardRenderPass::~ezTransparentForwardRenderPass()
{
}

void ezTransparentForwardRenderPass::RenderObjects(const ezRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitTransparent);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);
}
