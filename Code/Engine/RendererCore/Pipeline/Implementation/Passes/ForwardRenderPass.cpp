#include <RendererCorePCH.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/ClusteredDataProvider.h>
#include <RendererCore/Pipeline/Passes/ForwardRenderPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezForwardRenderPass, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil)->AddAttributes(new ezColorAttribute(ezColor::LightCoral)),
    EZ_ENUM_MEMBER_PROPERTY("ShadingQuality", ezForwardRenderShadingQuality, m_ShadingQuality)->AddAttributes(new ezDefaultValueAttribute((int)ezForwardRenderShadingQuality::Normal)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezForwardRenderShadingQuality, 1)
  EZ_ENUM_CONSTANTS(ezForwardRenderShadingQuality::Normal, ezForwardRenderShadingQuality::Simplified)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezForwardRenderPass::ezForwardRenderPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
  , m_ShadingQuality(ezForwardRenderShadingQuality::Normal)
{
}

ezForwardRenderPass::~ezForwardRenderPass() {}

bool ezForwardRenderPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
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

  return true;
}

void ezForwardRenderPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  SetupResources(renderViewContext, inputs, outputs);
  SetupPermutationVars(renderViewContext);
  SetupLighting(renderViewContext);

  RenderObjects(renderViewContext);

  ezRenderContext::EndPassAndRendering(renderViewContext);
}

void ezForwardRenderPass::SetupResources(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderingSetup renderingSetup;
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle));
  }

  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  ezRenderContext::BeginPassAndRendering(renderViewContext, std::move(renderingSetup), GetName());
}

void ezForwardRenderPass::SetupPermutationVars(const ezRenderViewContext& renderViewContext)
{
  ezTempHashedString sRenderPass("RENDER_PASS_FORWARD");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != ezViewRenderMode::None)
  {
    sRenderPass = ezViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  ezStringBuilder sDebugText;
  ezViewRenderMode::GetDebugText(renderViewContext.m_pViewData->m_ViewRenderMode, sDebugText);
  if (!sDebugText.IsEmpty())
  {
    ezDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sDebugText, ezVec2I32(10, 10), ezColor::White);
  }

  // Set permutation for shading quality
  if (m_ShadingQuality == ezForwardRenderShadingQuality::Normal)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");
  }
  else if (m_ShadingQuality == ezForwardRenderShadingQuality::Simplified)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_SIMPLIFIED");
  }
  else
  {
    EZ_REPORT_FAILURE("Unknown shading quality setting.");
  }
}

void ezForwardRenderPass::SetupLighting(const ezRenderViewContext& renderViewContext)
{
  // Setup clustered data
  if (m_ShadingQuality == ezForwardRenderShadingQuality::Normal)
  {
    auto pClusteredData = GetPipeline()->GetFrameDataProvider<ezClusteredDataProvider>()->GetData(renderViewContext);
    pClusteredData->BindResources(renderViewContext.m_pRenderContext);
  }
  // Or other light properties.
  else
  {
    // todo
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ForwardRenderPass);
