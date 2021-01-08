#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/DepthOnlyPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDepthOnlyPass, 1, ezRTTIDefaultAllocator<ezDepthOnlyPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil)->AddAttributes(new ezColorAttribute(ezColor::LightCoral)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDepthOnlyPass::ezDepthOnlyPass(const char* szName)
  : ezRenderPipelinePass(szName)
{
}

ezDepthOnlyPass::~ezDepthOnlyPass() {}

bool ezDepthOnlyPass::GetRenderTargetDescriptions(
  const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
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

void ezDepthOnlyPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderingSetup renderingSetup;
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  auto pCommandEncoder = ezRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName());

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");

  // Render
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaque);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMasked);
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_DepthOnlyPass);
