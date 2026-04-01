#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/LensEffectsPass.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLensEffectsPass, 1, ezRTTIDefaultAllocator<ezLensEffectsPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("ResolvedDepth", m_PinResolvedDepth),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects")
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLensEffectsPass::ezLensEffectsPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezLensEffectsPass::~ezLensEffectsPass() = default;

bool ezLensEffectsPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }
  else
  {
    ezLog::Error("No color input connected to pass '{0}'!", GetName());
    return false;
  }

  return true;
}

void ezLensEffectsPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderingSetup renderingSetup;
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    renderingSetup.SetColorTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle));
  }

  renderViewContext.m_pRenderContext->BeginRendering(std::move(renderingSetup), renderViewContext.m_pViewData->m_ViewPortRect, "", renderViewContext.m_pCamera->IsStereoscopic());

  if (inputs[m_PinResolvedDepth.m_uiInputIndex])
  {
    ezBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
    bindGroupRenderPass.BindTexture("SceneDepth", inputs[m_PinResolvedDepth.m_uiInputIndex]->m_TextureHandle);
  }

  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LensEffects);

  renderViewContext.m_pRenderContext->EndRendering();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_LensEffectsPass);
