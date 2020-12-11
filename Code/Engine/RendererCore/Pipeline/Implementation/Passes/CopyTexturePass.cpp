#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/CopyTexturePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCopyTexturePass, 1, ezRTTIDefaultAllocator<ezCopyTexturePass>)
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

ezCopyTexturePass::ezCopyTexturePass()
  : ezRenderPipelinePass("CopyTexturePass")
{
}

ezCopyTexturePass::~ezCopyTexturePass() = default;

bool ezCopyTexturePass::GetRenderTargetDescriptions(
  const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  const ezGALTextureCreationDescription* pInput = inputs[m_PinInput.m_uiInputIndex];

  if (pInput != nullptr)
  {
    ezGALTextureCreationDescription desc = *pInput;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    ezLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void ezCopyTexturePass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  
  const ezGALTexture* pDest = pDevice->GetTexture(pOutput->m_TextureHandle);
  const ezGALTexture* pSource = pDevice->GetTexture(pInput->m_TextureHandle);

  if (pDest->GetDescription().m_Format != pSource->GetDescription().m_Format)
  {
    // TODO: use a shader when the format doesn't match exactly

    ezLog::Error("Copying textures of different formats is not implemented");
  }
  else
  {
    auto pCommandEncoder = ezRenderContext::BeginPassAndComputeScope(renderViewContext, GetName());

    pCommandEncoder->CopyTexture(pOutput->m_TextureHandle, pInput->m_TextureHandle);
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_CopyTexturePass);
