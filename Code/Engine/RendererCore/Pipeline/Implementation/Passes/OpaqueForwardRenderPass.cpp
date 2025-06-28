#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/OpaqueForwardRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezOpaqueForwardRenderPass, 1, ezRTTIDefaultAllocator<ezOpaqueForwardRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SSAO", m_PinSSAO),
    EZ_MEMBER_PROPERTY("WriteDepth", m_bWriteDepth)->AddAttributes(new ezDefaultValueAttribute(true)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezOpaqueForwardRenderPass::ezOpaqueForwardRenderPass(const char* szName)
  : ezForwardRenderPass(szName)

{
  m_hWhiteTexture = ezResourceManager::LoadResource<ezTexture2DResource>("White.color");
}

ezOpaqueForwardRenderPass::~ezOpaqueForwardRenderPass() = default;

bool ezOpaqueForwardRenderPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  if (!SUPER::GetRenderTargetDescriptions(view, inputs, outputs))
  {
    return false;
  }

  if (inputs[m_PinSSAO.m_uiInputIndex])
  {
    if (inputs[m_PinSSAO.m_uiInputIndex]->m_uiWidth != inputs[m_PinColor.m_uiInputIndex]->m_uiWidth ||
        inputs[m_PinSSAO.m_uiInputIndex]->m_uiHeight != inputs[m_PinColor.m_uiInputIndex]->m_uiHeight)
    {
      ezLog::Warning("Expected same resolution for SSAO and color input to pass '{0}'!", GetName());
    }

    if (m_ShadingQuality == ezForwardRenderShadingQuality::Simplified)
    {
      ezLog::Warning("SSAO input will be ignored for pass '{0}' since simplified shading is activated.", GetName());
    }
  }

  return true;
}

void ezOpaqueForwardRenderPass::SetupResources(ezGALCommandEncoder* pCommandEncoder, const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  SUPER::SetupResources(pCommandEncoder, renderViewContext, inputs, outputs);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
  // SSAO texture
  if (m_ShadingQuality == ezForwardRenderShadingQuality::Normal)
  {
    if (inputs[m_PinSSAO.m_uiInputIndex])
    {
      bindGroupRenderPass.BindTexture("SSAOTexture", inputs[m_PinSSAO.m_uiInputIndex]->m_TextureHandle);
    }
    else
    {
      bindGroupRenderPass.BindTexture("SSAOTexture", m_hWhiteTexture, ezResourceAcquireMode::BlockTillLoaded);
    }
  }
}

void ezOpaqueForwardRenderPass::SetupPermutationVars(const ezRenderViewContext& renderViewContext)
{
  SUPER::SetupPermutationVars(renderViewContext);

  if (m_bWriteDepth)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "TRUE");
  }
  else
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "FALSE");
  }
}

void ezOpaqueForwardRenderPass::RenderObjects(const ezRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaqueStatic);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaqueDynamic);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMaskedStatic);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMaskedDynamic);
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_OpaqueForwardRenderPass);
