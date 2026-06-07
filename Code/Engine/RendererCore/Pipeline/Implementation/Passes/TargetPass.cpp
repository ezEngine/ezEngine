#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/TargetPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetView.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTargetPass, 1, ezRTTIDefaultAllocator<ezTargetPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color0", m_PinColor0),
    EZ_MEMBER_PROPERTY("Color1", m_PinColor1),
    EZ_MEMBER_PROPERTY("Color2", m_PinColor2),
    EZ_MEMBER_PROPERTY("Color3", m_PinColor3),
    EZ_MEMBER_PROPERTY("Color4", m_PinColor4),
    EZ_MEMBER_PROPERTY("Color5", m_PinColor5),
    EZ_MEMBER_PROPERTY("Color6", m_PinColor6),
    EZ_MEMBER_PROPERTY("Color7", m_PinColor7),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Output")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTargetPass::ezTargetPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezTargetPass::~ezTargetPass() = default;

ezStatus ezTargetPass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  m_hSwapChain = viewData.m_hSwapChain;
  m_RenderTargets = viewData.m_RenderTargets;

  ezTempHashedString pinNames[] = {
    "Color0",
    "Color1",
    "Color2",
    "Color3",
    "Color4",
    "Color5",
    "Color6",
    "Color7",
    "DepthStencil",
  };

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(pinNames); ++i)
  {
    EZ_SUCCEED_OR_RETURN(VerifyInput(ref_graph, inputs, pinNames[i]));
  }

  return EZ_SUCCESS;
}

ezGALTextureHandle ezTargetPass::QueryTextureProvider(const ezRenderPipelineNodePin* pPin, const ezGALTextureCreationDescription& desc)
{
  EZ_ASSERT_DEV(pPin->m_pParent == this, "ezTargetPass::QueryTextureProvider: The given pin is not part of this pass!");

  auto GetActiveRenderTargets = [&]() -> const ezGALRenderTargets&
  {
    if (const ezGALSwapChain* pSwapChain = ezGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain))
    {
      return pSwapChain->GetRenderTargets();
    }
    return m_RenderTargets;
  };
  const ezGALRenderTargets& renderTargets = GetActiveRenderTargets();

  ezGALTextureHandle hTarget;
  if (pPin->m_uiInputIndex == 8)
  {
    return renderTargets.m_hDSTarget;
  }
  else
  {
    return renderTargets.m_hRTs[pPin->m_uiInputIndex];
  }
}

ezStatus ezTargetPass::VerifyInput(ezRenderGraph& graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezTempHashedString sPinName)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  const ezRenderPipelineNodePin* pPin = GetPinByName(sPinName);
  if (inputs[pPin->m_uiInputIndex].m_Connectivity == ezRenderPipelinePinConnection::Connectivity::Texture)
  {
    const ezGALTextureCreationDescription& desc = graph.GetTextureDesc(inputs[pPin->m_uiInputIndex].m_TextureHandle);
    const ezGALTextureHandle handle = QueryTextureProvider(pPin, desc);
    if (!handle.IsInvalidated())
    {
      const ezGALTexture* pTexture = pDevice->GetTexture(handle);
      if (pTexture)
      {
        // TODO: Need a more sophisticated check here what is considered 'matching'
        // if (inputs[pPin->m_uiInputIndex]->CalculateHash() != pTexture->GetDescription().CalculateHash())
        //  return false;
      }
    }
  }

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TargetPass);
