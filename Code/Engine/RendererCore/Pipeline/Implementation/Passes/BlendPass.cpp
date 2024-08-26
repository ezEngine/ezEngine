#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/BlendPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BlendConstants.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBlendPass, 1, ezRTTIDefaultAllocator<ezBlendPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InputA", m_PinInputA),
    EZ_MEMBER_PROPERTY("InputB", m_PinInputB),
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_MEMBER_PROPERTY("BlendFactor", m_fBlendFactor)->AddAttributes(new ezDefaultValueAttribute(0.5f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBlendPass::ezBlendPass()
  : ezRenderPipelinePass("BlendPass")
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/Blend.ezShader");
  EZ_ASSERT_DEV(m_hShader.IsValid(), "Could not load blend shader!");
}

ezBlendPass::~ezBlendPass() = default;

bool ezBlendPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInputA.m_uiInputIndex] && inputs[m_PinInputB.m_uiInputIndex])
  {
    if (!inputs[m_PinInputA.m_uiInputIndex]->m_bAllowShaderResourceView || !inputs[m_PinInputB.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      ezLog::Error("Blend pass inputs must allow shader resource view.");
      return false;
    }

    outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinInputA.m_uiInputIndex];
  }
  else
  {
    ezLog::Error("No input connected to blend pass!");
    return false;
  }

  return true;
}

void ezBlendPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  if (outputs[m_PinOutput.m_uiOutputIndex])
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

    ezBlendConstants cb = {};
    cb.BlendFactor = m_fBlendFactor;
    renderViewContext.m_pRenderContext->SetPushConstants("ezBlendConstants", cb);

    // Setup render target
    ezGALRenderingSetup renderingSetup;
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
    renderingSetup.m_uiRenderTargetClearMask = ezInvalidIndex;
    renderingSetup.m_ClearColor = ezColor(1.0f, 0.0f, 0.0f);

    // Bind render target and viewport
    auto pCommandEncoder = ezRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, GetName(), renderViewContext.m_pCamera->IsStereoscopic());

    // Setup input view and sampler
    ezGALTextureResourceViewCreationDescription rvcd;
    rvcd.m_hTexture = inputs[m_PinInputA.m_uiInputIndex]->m_TextureHandle;
    ezGALTextureResourceViewHandle hResourceViewA = ezGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);
    rvcd.m_hTexture = inputs[m_PinInputB.m_uiInputIndex]->m_TextureHandle;
    ezGALTextureResourceViewHandle hResourceViewB = ezGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("InputA", hResourceViewA);
    renderViewContext.m_pRenderContext->BindTexture2D("InputB", hResourceViewB);

    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
}

ezResult ezBlendPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fBlendFactor;
  return EZ_SUCCESS;
}

ezResult ezBlendPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fBlendFactor;
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_BlendPass);
