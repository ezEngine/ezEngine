#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/AntialiasingPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAntialiasingPass, 1, ezRTTIDefaultAllocator<ezAntialiasingPass>)
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

ezAntialiasingPass::ezAntialiasingPass()
  : ezRenderPipelinePass("AntialiasingPass")
{
  {
    // Load shader.
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Antialiasing.ezShader");
    EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load antialiasing shader!");
  }
}

ezAntialiasingPass::~ezAntialiasingPass() {}

bool ezAntialiasingPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_SampleCount == ezGALMSAASampleCount::TwoSamples)
    {
      m_sMsaaSampleCount.Assign("MSAA_SAMPLES_TWO");
    }
    else if (pInput->m_SampleCount == ezGALMSAASampleCount::FourSamples)
    {
      m_sMsaaSampleCount.Assign("MSAA_SAMPLES_FOUR");
    }
    else if (pInput->m_SampleCount == ezGALMSAASampleCount::EightSamples)
    {
      m_sMsaaSampleCount.Assign("MSAA_SAMPLES_EIGHT");
    }
    else
    {
      ezLog::Error("Input is not a valid msaa target");
      return false;
    }

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

void ezAntialiasingPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
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
  auto commandEncoderScope = ezRenderContext::BeginRendering(renderViewContext, std::move(renderingSetup), GetName());

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("MSAA_SAMPLES", m_sMsaaSampleCount);

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_AntialiasingPass);
