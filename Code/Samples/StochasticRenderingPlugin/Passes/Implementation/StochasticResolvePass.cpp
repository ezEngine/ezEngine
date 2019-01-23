#include <PCH.h>

#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Resource.h>
#include <RendererFoundation/Resources/Texture.h>
#include <StochasticRenderingPlugin/Passes/StochasticResolvePass.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStochasticResolvePass, 1, ezRTTIDefaultAllocator<ezStochasticResolvePass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("StochasticColor", m_PinStochasticColor),
    EZ_MEMBER_PROPERTY("SampleCount", m_PinSampleCount)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStochasticResolvePass::ezStochasticResolvePass(const char* name /* = "StoachsticResolvePass" */)
    : ezRenderPipelinePass(name)
{
  // Load shader.
  m_hResolveShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/StochasticResolve.ezShader");
  EZ_ASSERT_DEV(m_hResolveShader.IsValid(), "Could not load stochastic resolve shader!");
}

ezStochasticResolvePass::~ezStochasticResolvePass() {}

bool ezStochasticResolvePass::GetRenderTargetDescriptions(const ezView& view,
                                                          const ezArrayPtr<ezGALTextureCreationDescription* const> inputs,
                                                          ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }
  else
  {
    return false;
  }

  return true;
}

void ezStochasticResolvePass::Execute(const ezRenderViewContext& renderViewContext,
                                      const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
                                      const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderTagetSetup renderTargetSetup;
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle));
  }

  renderViewContext.m_pRenderContext->SetViewportAndRenderTargetSetup(renderViewContext.m_pViewData->m_ViewPortRect, renderTargetSetup);

  renderViewContext.m_pRenderContext->BindShader(m_hResolveShader);
  renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles,
                                                     1);
  renderViewContext.m_pRenderContext->BindTexture2D(
      "ColorTexture", pDevice->GetDefaultResourceView(inputs[m_PinStochasticColor.m_uiInputIndex]->m_TextureHandle));
  renderViewContext.m_pRenderContext->BindTexture2D(
      "SampleCountTexture", pDevice->GetDefaultResourceView(inputs[m_PinSampleCount.m_uiInputIndex]->m_TextureHandle));

  renderViewContext.m_pRenderContext->DrawMeshBuffer();
}
