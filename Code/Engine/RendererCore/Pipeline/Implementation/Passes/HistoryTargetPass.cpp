#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/HistoryTargetPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezHistoryTargetPass, 1, ezRTTIDefaultAllocator<ezHistoryTargetPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_PinInput),
    EZ_MEMBER_PROPERTY("SourcePassName", m_sSourcePassName)->AddAttributes(new ezDefaultValueAttribute("HistorySourcePass"))
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezHistoryTargetPass::ezHistoryTargetPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezHistoryTargetPass::~ezHistoryTargetPass() = default;

bool ezHistoryTargetPass::GetRenderTargetDescriptions(
  const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  auto pData = GetPipeline()->GetFrameDataProvider<ezHistorySourcePassTextureDataProvider>();
  pData->ResetTexture(m_sSourcePassName);
  return true;
}

ezGALTextureHandle ezHistoryTargetPass::QueryTextureProvider(const ezRenderPipelineNodePin* pPin, const ezGALTextureCreationDescription& desc)
{
  auto pData = GetPipeline()->GetFrameDataProvider<ezHistorySourcePassTextureDataProvider>();
  return pData->GetOrCreateTexture(m_sSourcePassName, desc);
}

void ezHistoryTargetPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
}

ezResult ezHistoryTargetPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_sSourcePassName;
  return EZ_SUCCESS;
}

ezResult ezHistoryTargetPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_sSourcePassName;
  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_HistoryTargetPass);

