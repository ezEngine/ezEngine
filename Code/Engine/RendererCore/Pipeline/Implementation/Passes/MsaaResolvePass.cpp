#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Passes/MsaaResolvePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsaaResolvePass, 1, ezRTTIDefaultAllocator<ezMsaaResolvePass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezMsaaResolvePass::ezMsaaResolvePass() : ezRenderPipelinePass("MsaaResolvePass")
{
}

ezMsaaResolvePass::~ezMsaaResolvePass()
{
}

bool ezMsaaResolvePass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs,
  ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_SampleCount == ezGALMSAASampleCount::None)
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
    ezLog::Error("No input connected to msaa resolve pass!");
    return false;
  }

  return true;
}

void ezMsaaResolvePass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  ezGALTextureSubresource subresource;
  subresource.m_uiMipLevel = 0;
  subresource.m_uiArraySlice = 0;

  pGALContext->ResolveTexture(pOutput->m_TextureHandle, subresource, pInput->m_TextureHandle, subresource);
}

