#include <PCH.h>
#include <RendererCore/Pipeline/Passes/TransparentForwardRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTransparentForwardRenderPass, 1, ezRTTIDefaultAllocator<ezTransparentForwardRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ResolvedDepth", m_PinResolvedDepth),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezTransparentForwardRenderPass::ezTransparentForwardRenderPass(const char* szName)
  : ezForwardRenderPass(szName)
{
}

ezTransparentForwardRenderPass::~ezTransparentForwardRenderPass()
{
}

void ezTransparentForwardRenderPass::SetupResources(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection * const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection * const> outputs)
{
  SUPER::SetupResources(renderViewContext, inputs, outputs);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  if (inputs[m_PinResolvedDepth.m_uiInputIndex])
  {
    ezGALResourceViewHandle depthResourceViewHandle = pDevice->GetDefaultResourceView(inputs[m_PinResolvedDepth.m_uiInputIndex]->m_TextureHandle);
    renderViewContext.m_pRenderContext->BindTexture2D(ezGALShaderStage::PixelShader, "SceneDepth", depthResourceViewHandle);
  }
}

void ezTransparentForwardRenderPass::RenderObjects(const ezRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitTransparent);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);
}
