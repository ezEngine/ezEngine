#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/MsaaUpscalePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsaaUpscalePass, 2, ezRTTIDefaultAllocator<ezMsaaUpscalePass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_ENUM_MEMBER_PROPERTY("MSAA_Mode", ezGALMSAASampleCount, m_MsaaMode)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Utilities")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezMsaaUpscalePass::ezMsaaUpscalePass()
  : ezRenderPipelinePass("MsaaUpscalePass")

{
  {
    // Load shader.
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/MsaaUpscale.ezShader");
    EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load msaa upscale shader!");
  }
}

ezMsaaUpscalePass::~ezMsaaUpscalePass() = default;

bool ezMsaaUpscalePass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  auto pInput = inputs[m_PinInput.m_uiInputIndex];
  if (pInput != nullptr)
  {
    if (pInput->m_SampleCount != ezGALMSAASampleCount::None)
    {
      ezLog::Error("Input must not be a msaa target");
      return false;
    }

    ezGALTextureCreationDescription desc = *pInput;
    desc.m_SampleCount = m_MsaaMode;

    outputs[m_PinOutput.m_uiOutputIndex] = desc;
  }
  else
  {
    ezLog::Error("No input connected to '{0}'!", GetName());
    return false;
  }

  return true;
}

void ezMsaaUpscalePass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
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
  renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));

  // Bind render target and viewport
  auto pCommandEncoder = ezRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
  renderViewContext.m_pRenderContext->BindTexture2D("ColorTexture", pDevice->GetDefaultResourceView(pInput->m_TextureHandle));

  renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
}

ezResult ezMsaaUpscalePass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_MsaaMode;
  return EZ_SUCCESS;
}

ezResult ezMsaaUpscalePass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_MsaaMode;
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezMsaaUpscalePassPatch_1_2 : public ezGraphPatch
{
public:
  ezMsaaUpscalePassPatch_1_2()
    : ezGraphPatch("ezMsaaUpscalePass", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override { pNode->RenameProperty("MSAA Mode", "MSAA_Mode"); }
};

ezMsaaUpscalePassPatch_1_2 g_ezMsaaUpscalePassPatch_1_2;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_MsaaUpscalePass);
