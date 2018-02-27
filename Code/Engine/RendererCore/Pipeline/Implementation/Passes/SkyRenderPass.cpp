#include <PCH.h>
#include <RendererCore/Pipeline/Passes/SkyRenderPass.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkyRenderPass, 1, ezRTTIDefaultAllocator<ezSkyRenderPass>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSkyRenderPass::ezSkyRenderPass(const char* szName)
  : ezForwardRenderPass(szName)
{
}

ezSkyRenderPass::~ezSkyRenderPass()
{
}

void ezSkyRenderPass::RenderObjects(const ezRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Sky);
}
