#include <PCH.h>
#include <RendererCore/Lights/ClusteredDataProvider.h>
#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezForwardRenderPass, 1, ezRTTIDefaultAllocator<ezForwardRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    EZ_MEMBER_PROPERTY("SSAO", m_PinSSAO),
    EZ_MEMBER_PROPERTY("WriteDepth", m_bWriteDepth)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("ApplySSAOToDirectLighting", m_applySSAOToDirectLight),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezForwardRenderPass::ezForwardRenderPass(const char* szName)
  : ezRenderPipelinePass(szName)
  , m_bWriteDepth(true)
  , m_applySSAOToDirectLight(false)
{
  m_hWhiteTexture = ezResourceManager::LoadResource<ezTexture2DResource>("White.color");
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

  SetupPermutationVars(renderViewContext);

  if (m_bWriteDepth)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("WRITE_DEPTH", "TRUE");
  }
  else
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("WRITE_DEPTH", "FALSE");
  }

  // Setup clustered data
  auto pClusteredData = GetPipeline()->GetFrameDataProvider<ezClusteredDataProvider>()->GetData(renderViewContext);
  pClusteredData->BindResources(renderViewContext.m_pRenderContext);

  // SSAO texture
  {
    if (inputs[m_PinSSAO.m_uiInputIndex])
    {
      ezGALResourceViewHandle ssaoResourceViewHandle = pDevice->GetDefaultResourceView(inputs[m_PinSSAO.m_uiInputIndex]->m_TextureHandle);
      renderViewContext.m_pRenderContext->BindTexture2D(ezGALShaderStage::PixelShader, "SSAOTexture", ssaoResourceViewHandle);

      if (m_applySSAOToDirectLight)
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("APPLY_SSAO_TO_DIRECT_LIGHTING", "TRUE");
      else
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("APPLY_SSAO_TO_DIRECT_LIGHTING", "FALSE");
    }
    else
    {
      renderViewContext.m_pRenderContext->BindTexture2D(ezGALShaderStage::PixelShader, "SSAOTexture", m_hWhiteTexture, ezResourceAcquireMode::NoFallback);
    }
  }

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

void ezForwardRenderPass::SetupPermutationVars(const ezRenderViewContext& renderViewContext)
{
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "FORWARD");
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ForwardRenderPass);

