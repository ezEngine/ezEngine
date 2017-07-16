#include <PCH.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Core/Graphics/Camera.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSourcePass, 2, ezRTTIDefaultAllocator<ezSourcePass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_ENUM_MEMBER_PROPERTY("Format", ezGALResourceFormat, m_Format),
    EZ_ENUM_MEMBER_PROPERTY("MSAA_Mode", ezGALMSAASampleCount, m_MsaaMode),
    EZ_MEMBER_PROPERTY("ClearColor", m_ClearColor),
    EZ_MEMBER_PROPERTY("Clear", m_bClear),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSourcePass::ezSourcePass(const char* szName) : ezRenderPipelinePass(szName, true)
{
  m_Format = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
  m_MsaaMode = ezGALMSAASampleCount::None;
  m_bClear = true;
}

ezSourcePass::~ezSourcePass()
{

}

bool ezSourcePass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  ezUInt32 uiWidth = static_cast<ezUInt32>(view.GetViewport().width);
  ezUInt32 uiHeight = static_cast<ezUInt32>(view.GetViewport().height);

  ezGALTextureCreationDescription desc;
  desc.m_uiWidth = uiWidth;
  desc.m_uiHeight = uiHeight;
  desc.m_SampleCount = m_MsaaMode;
  desc.m_Format = m_Format;
  desc.m_bCreateRenderTarget = true;
  desc.m_uiArraySize = view.GetCamera()->IsStereoscopic() ? 2 : 1;

  outputs[m_PinOutput.m_uiOutputIndex] = desc;

  return true;
}

void ezSourcePass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  if (!m_bClear)
    return;

  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  // Setup render target
  ezGALRenderTagetSetup renderTargetSetup;
  if (ezGALResourceFormat::IsDepthFormat(m_Format))
  {
    renderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  }
  else
  {
    renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  }

  pGALContext->SetRenderTargetSetup(renderTargetSetup);

  // Clear color or depth stencil
  pGALContext->Clear(m_ClearColor);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezSourcePassPatch_1_2 : public ezGraphPatch
{
public:
  ezSourcePassPatch_1_2()
    : ezGraphPatch("ezSourcePass", 2) {}

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("MSAA Mode", "MSAA_Mode");
    pNode->RenameProperty("Clear Color", "ClearColor");
  }
};

ezSourcePassPatch_1_2 g_ezSourcePassPatch_1_2;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SourcePass);

