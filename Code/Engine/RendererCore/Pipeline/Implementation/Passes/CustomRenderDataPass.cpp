#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/CustomRenderDataPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomRenderDataPass, 1, ezRTTIDefaultAllocator<ezCustomRenderDataPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    EZ_MEMBER_PROPERTY("RenderDataCategory", m_sRenderDataCategoryName),
    EZ_ENUM_MEMBER_PROPERTY("SortingFunction", ezRenderSortingFunctions, m_SortingFunction),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCustomRenderDataPass::ezCustomRenderDataPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezCustomRenderDataPass::~ezCustomRenderDataPass() = default;

bool ezCustomRenderDataPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }

  // DepthStencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }

  return true;
}

void ezCustomRenderDataPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  if (m_RenderDataCategory.IsValid() == false)
    return;

  auto batchList = GetPipeline()->GetRenderDataBatchesWithCategory(m_RenderDataCategory);
  if (batchList.GetBatchCount() == 0)
    return;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderingSetup renderingSetup;
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle));
  }

  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  renderViewContext.m_pRenderContext->BeginRendering(std::move(renderingSetup), renderViewContext.m_pViewData->m_ViewPortRect, "", renderViewContext.m_pCamera->IsStereoscopic());

  RenderDataWithCategory(renderViewContext, m_RenderDataCategory);

  renderViewContext.m_pRenderContext->EndRendering();
}

ezResult ezCustomRenderDataPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_sRenderDataCategoryName;
  inout_stream << m_SortingFunction;
  return EZ_SUCCESS;
}

ezResult ezCustomRenderDataPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_sRenderDataCategoryName;
  inout_stream >> m_SortingFunction;

  if (m_sRenderDataCategoryName.IsEmpty() == false)
  {
    m_RenderDataCategory = ezRenderData::RegisterCategory(m_sRenderDataCategoryName, ezRenderSortingFunctions::GetFunction(m_SortingFunction));
  }

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_CustomRenderDataPass);
