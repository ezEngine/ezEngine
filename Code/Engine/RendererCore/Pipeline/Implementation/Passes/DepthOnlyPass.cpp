#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/DepthOnlyPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <Foundation/IO/TypeVersionContext.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDepthOnlyPass, 3, ezRTTIDefaultAllocator<ezDepthOnlyPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    EZ_MEMBER_PROPERTY("RenderStaticObjects", m_bRenderStaticObjects)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("RenderDynamicObjects", m_bRenderDynamicObjects)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("RenderTransparentObjects", m_bRenderTransparentObjects),
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

ezDepthOnlyPass::ezDepthOnlyPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezDepthOnlyPass::~ezDepthOnlyPass() = default;

bool ezDepthOnlyPass::GetRenderTargetDescriptions(
  const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  // DepthStencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    ezLog::Error("No depth stencil input connected to pass '{0}'!", GetName());
    return false;
  }

  return true;
}

void ezDepthOnlyPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderingSetup renderingSetup;
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  auto pCommandEncoder = ezRenderContext::BeginRenderingScope(renderViewContext, renderingSetup, GetName(), renderViewContext.m_pCamera->IsStereoscopic());

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_DEPTH_ONLY");
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHADING_QUALITY", "SHADING_QUALITY_NORMAL");

  // Opaque
  if (m_bRenderStaticObjects)
  {
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaqueStatic);
  }
  if (m_bRenderDynamicObjects)
  {
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaqueDynamic);
  }

  // Masked
  if (m_bRenderStaticObjects)
  {
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMaskedStatic);
  }
  if (m_bRenderDynamicObjects)
  {
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMaskedDynamic);
  }

  // Transparent
  if (m_bRenderTransparentObjects)
  {
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitTransparent);
  }
}

ezResult ezDepthOnlyPass::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_bRenderStaticObjects;
  inout_stream << m_bRenderDynamicObjects;
  inout_stream << m_bRenderTransparentObjects;
  return EZ_SUCCESS;
}

ezResult ezDepthOnlyPass::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());

  if (uiVersion >= 3)
  {
    inout_stream >> m_bRenderStaticObjects;
    inout_stream >> m_bRenderDynamicObjects;
  }

  if (uiVersion >= 2)
  {
    inout_stream >> m_bRenderTransparentObjects;
  }

  return EZ_SUCCESS;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_DepthOnlyPass);
