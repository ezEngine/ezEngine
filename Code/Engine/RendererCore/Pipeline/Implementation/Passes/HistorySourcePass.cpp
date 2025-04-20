#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/HistorySourcePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezHistorySourcePassTextureDataProvider, 1, ezRTTIDefaultAllocator<ezHistorySourcePassTextureDataProvider>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezHistorySourcePass, 1, ezRTTIDefaultAllocator<ezHistorySourcePass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Output", m_PinOutput),
    EZ_ENUM_MEMBER_PROPERTY("Format", ezSourceFormat, m_Format),
    EZ_ENUM_MEMBER_PROPERTY("MSAA_Mode", ezGALMSAASampleCount, m_MsaaMode),
    EZ_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new ezExposeColorAlphaAttribute())
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

bool ezHistorySourcePass::GetRenderTargetDescriptions(
  const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  auto pData = GetPipeline()->GetFrameDataProvider<ezHistorySourcePassTextureDataProvider>();
  pData->ResetTexture(GetName());
  m_bFirstExecute = true;
  outputs[m_PinOutput.m_uiOutputIndex] = ezSourcePass::GetOutputDescription(view, m_Format, m_MsaaMode);
  return true;
}

ezGALTextureHandle ezHistorySourcePass::QueryTextureProvider(const ezRenderPipelineNodePin* pPin, const ezGALTextureCreationDescription& desc)
{
  auto pData = GetPipeline()->GetFrameDataProvider<ezHistorySourcePassTextureDataProvider>();
  return pData->GetOrCreateTexture(GetName(), desc);
}

void ezHistorySourcePass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr || !m_bFirstExecute)
    return;

  m_bFirstExecute = false;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderingSetup renderingSetup;
  if (ezGALResourceFormat::IsDepthFormat(pOutput->m_Desc.m_Format))
  {
    renderingSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
    renderingSetup.SetClearDepth().SetClearStencil();
  }
  else
  {
    renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
    renderingSetup.SetClearColor(0, m_ClearColor);
  }

  auto pCommandEncoder = ezRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, GetName());
}

ezResult ezHistorySourcePass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_Format;
  inout_stream << m_MsaaMode;
  inout_stream << m_ClearColor;
  return EZ_SUCCESS;
}

ezResult ezHistorySourcePass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_Format;
  inout_stream >> m_MsaaMode;
  inout_stream >> m_ClearColor;
  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_HistorySourcePass);
