#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>
#include <RendererCore/Pipeline/DataProviders/ClusteredDataProvider.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezForwardRenderPass, 1, ezRTTIDefaultAllocator<ezForwardRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    EZ_MEMBER_PROPERTY("ScreenSpaceAmbientObscurance", m_PinSSAO),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezForwardRenderPass::ezForwardRenderPass(const char* szName) : ezRenderPipelinePass(szName)
{
}

ezForwardRenderPass::~ezForwardRenderPass()
{
}

bool ezForwardRenderPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs,
  ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }
  else
  {
    ezLog::Error("No color input connected to pass '{0}'!", GetName());
    return false;
  }
  
  // DepthStencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    ezLog::Error("No depth stencil input connected to pass '{0}'!", GetName());
    return false;
  }

  if (inputs[m_PinSSAO.m_uiInputIndex])
  {
    if (inputs[m_PinSSAO.m_uiInputIndex]->m_uiWidth != inputs[m_PinColor.m_uiInputIndex]->m_uiWidth ||
        inputs[m_PinSSAO.m_uiInputIndex]->m_uiHeight != inputs[m_PinColor.m_uiInputIndex]->m_uiHeight)
    {
      ezLog::Warning("Expected same resolution for SSAO and color input to pass '{0}'!", GetName());
    }
  }

  return true;
}

void ezForwardRenderPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderTagetSetup renderTargetSetup;
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle));
  }

  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  renderViewContext.m_pRenderContext->SetViewportAndRenderTargetSetup(renderViewContext.m_pViewData->m_ViewPortRect, renderTargetSetup);

  bool enableSSAO = (inputs[m_PinSSAO.m_uiInputIndex] != nullptr);
  SetupPermutationVars(renderViewContext, enableSSAO);

  // Set SSAO texture (optional)
  if (enableSSAO)
  {
    ezGALResourceViewHandle ssaoResourceViewHandle = pDevice->GetDefaultResourceView(inputs[m_PinSSAO.m_uiInputIndex]->m_TextureHandle);
    EZ_ASSERT_DEV(ssaoResourceViewHandle != ezGALResourceViewHandle(), "There is no resource view for the SSAO output texture.");
    renderViewContext.m_pRenderContext->BindTexture2D(ezGALShaderStage::PixelShader, "SSAOTexture", ssaoResourceViewHandle);
  }

  // Setup clustered data
  ezClusteredData* pClusteredData = GetPipeline()->GetFrameDataProvider<ezClusteredDataProvider>()->GetData(renderViewContext);
  pClusteredData->BindResources(renderViewContext.m_pRenderContext);

  // Optionally enable SSAO.
  if (inputs[m_PinSSAO.m_uiInputIndex])
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("USE_SSAO", "TRUE");
    ezGALResourceViewHandle ssaoResourceViewHandle = pDevice->GetDefaultResourceView(inputs[m_PinSSAO.m_uiInputIndex]->m_TextureHandle);
    EZ_ASSERT_DEV(ssaoResourceViewHandle != ezGALResourceViewHandle(), "There is no resource view for the SSAO output texture.");
    renderViewContext.m_pRenderContext->BindTexture2D(ezGALShaderStage::PixelShader, "SSAOTexture", ssaoResourceViewHandle);
  }
  else
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("USE_SSAO", "FALSE");

  // Render
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaque);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMasked);

  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Sky);

  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitTransparent);
  
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);
}

void ezForwardRenderPass::SetupPermutationVars(const ezRenderViewContext& renderViewContext, bool enableSSAO)
{
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "FORWARD");

  if(enableSSAO)
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("USE_SSAO", "TRUE");
  else
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("USE_SSAO", "FALSE");
}
