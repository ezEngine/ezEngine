#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/StereoTestPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <Core/Graphics/Camera.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStereoTestPass, 1, ezRTTIDefaultAllocator<ezStereoTestPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezStereoTestPass::ezStereoTestPass()
  : ezRenderPipelinePass("StereoTestPass", true)
{
  {
    // Load shader.
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/StereoTest.ezShader");
    EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load stereo test shader!");
  }
}

ezStereoTestPass::~ezStereoTestPass() {}

bool ezStereoTestPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    ezGALTextureCreationDescription desc = *pInput;
    desc.m_SampleCount = ezGALMSAASampleCount::None;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    ezLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void ezStereoTestPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));

  // Bind render target and viewport
  auto pCommandEncoder = ezRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());

  renderViewContext.m_pRenderContext->BindShader(m_hShader);

  renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

  renderViewContext.m_pRenderContext->DrawMeshBuffer(0xFFFFFFFF, 0, renderViewContext.m_pCamera->IsStereoscopic() ? 2 : 1).IgnoreResult();
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_StereoTestPass);
