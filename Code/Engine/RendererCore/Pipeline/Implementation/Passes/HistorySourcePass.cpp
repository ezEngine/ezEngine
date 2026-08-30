#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/HistorySourcePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezHistorySourcePassTextureDataProvider, 1, ezRTTIDefaultAllocator<ezHistorySourcePassTextureDataProvider>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezHistorySourcePass, 2, ezRTTIDefaultAllocator<ezHistorySourcePass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_ENUM_MEMBER_PROPERTY("Type", ezRequiredTextureType, m_Type),
    EZ_ENUM_MEMBER_PROPERTY("Precision", ezRequiredTexturePrecision, m_MinPrecision),
    EZ_ENUM_MEMBER_PROPERTY("Channels", ezRequiredTextureChannels, m_MinChannels),
    EZ_ENUM_MEMBER_PROPERTY("MSAA_Mode", ezGALMSAASampleCount, m_MsaaMode),
    EZ_MEMBER_PROPERTY("UAV", m_bUAV),
    EZ_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_MEMBER_PROPERTY("ClearDepth", m_fClearDepth)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0f, 1.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezHistorySourcePassTextureDataProvider::ezHistorySourcePassTextureDataProvider() = default;
ezHistorySourcePassTextureDataProvider::~ezHistorySourcePassTextureDataProvider()
{
  while (!m_Data.IsEmpty())
  {
    ResetTexture(m_Data.GetIterator().Key());
  }
}

void ezHistorySourcePassTextureDataProvider::ResetTexture(ezStringView sSourcePassName)
{
  if (ezGALTextureHandle* pHandle = m_Data.GetValue(sSourcePassName))
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    pDevice->DestroyTexture(*pHandle);
    m_Data.Remove(sSourcePassName);
  }
}

ezGALTextureHandle ezHistorySourcePassTextureDataProvider::GetOrCreateTexture(ezStringView sSourcePassName, const ezGALTextureCreationDescription& desc)
{
  bool bExisted;
  ezGALTextureHandle& hTexture = m_Data.FindOrAdd(sSourcePassName, &bExisted);
  if (!bExisted)
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    hTexture = pDevice->CreateTexture(desc);
    if (hTexture.IsInvalidated())
      ezLog::Error("Failed to create history source pass texture");
  }
  return hTexture;
}


ezHistorySourcePass::ezHistorySourcePass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezHistorySourcePass::~ezHistorySourcePass() = default;

ezStatus ezHistorySourcePass::AddRenderPasses(const ezViewData& viewData, const ezCamera& camera, ezRenderGraph& ref_graph, const ezArrayPtr<const ezRenderPipelinePinConnection> inputs, ezArrayPtr<ezRenderPipelinePinConnection> outputs)
{
  auto pData = GetPipeline()->GetFrameDataProvider<ezHistorySourcePassTextureDataProvider>();
  pData->ResetTexture(GetName());
  ezGALTextureCreationDescription desc;
  EZ_SUCCEED_OR_RETURN(ezSourcePass::GetOutputDescription(viewData, camera, m_Type, m_MinPrecision, m_MinChannels, m_MsaaMode, m_bUAV, desc));
  ezGALTextureHandle hTexture = QueryTextureProvider(&m_PinOutput, desc);
  ezRenderGraphTextureHandle hGraphTexture = ref_graph.ImportTexture(hTexture);
  outputs[m_PinOutput.m_uiOutputIndex].m_TextureHandle = hGraphTexture;

  // When we encounter the texture for the first time, the history target has never been written before and we clear it.
  if (hTexture != m_hTextureCleared)
  {
    m_hTextureCleared = hTexture;
    auto pass = ref_graph.AddGraphicsPass(GetName());
    if (ezGALResourceFormat::IsDepthFormat(desc.m_Format))
    {
      pass.AddDepthStencilTarget(hGraphTexture, {}, ezGALRenderTargetLoadOp::Clear);
      pass.SetClearDepth().SetClearStencil();
      pass.SetClearDepth(m_fClearDepth);
    }
    else
    {
      pass.AddDepthStencilTarget(hGraphTexture, {}, ezGALRenderTargetLoadOp::Clear);
      pass.SetClearColor(0, m_ClearColor);
    }
  }

  return EZ_SUCCESS;
}

ezGALTextureHandle ezHistorySourcePass::QueryTextureProvider(const ezRenderPipelineNodePin* pPin, const ezGALTextureCreationDescription& desc)
{
  auto pData = GetPipeline()->GetFrameDataProvider<ezHistorySourcePassTextureDataProvider>();
  return pData->GetOrCreateTexture(GetName(), desc);
}

ezResult ezHistorySourcePass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_Type;
  inout_stream << m_MinPrecision;
  inout_stream << m_MinChannels;
  inout_stream << m_MsaaMode;
  inout_stream << m_ClearColor;
  inout_stream << m_fClearDepth;
  inout_stream << m_bUAV;
  return EZ_SUCCESS;
}

ezResult ezHistorySourcePass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  if (uiVersion >= 2)
  {
    inout_stream >> m_Type;
    inout_stream >> m_MinPrecision;
    inout_stream >> m_MinChannels;
    inout_stream >> m_MsaaMode;
    inout_stream >> m_ClearColor;
    inout_stream >> m_fClearDepth;
    inout_stream >> m_bUAV;
  }
  else
  {
    // Version 1 stored the removed ezSourceFormat enum, which used ezUInt8 as storage.
    ezUInt8 uiLegacyFormat = 0;
    inout_stream >> uiLegacyFormat;
    ezGetLegacyTextureFormatRequirements(uiLegacyFormat, false, m_Type, m_MinPrecision, m_MinChannels);
    inout_stream >> m_MsaaMode;
    inout_stream >> m_ClearColor;
  }

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

/// Replaces the removed ezSourceFormat 'Format' property with the texture requirement properties.
class ezHistorySourcePassPatch_1_2 : public ezGraphPatch
{
public:
  ezHistorySourcePassPatch_1_2()
    : ezGraphPatch("ezHistorySourcePass", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ezPatchLegacyTextureFormatProperty(pNode, false);
  }
};

ezHistorySourcePassPatch_1_2 g_ezHistorySourcePassPatch_1_2;


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_HistorySourcePass);
