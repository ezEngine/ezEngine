#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimpleRenderPass, 1, ezRTTIDefaultAllocator<ezSimpleRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil)->AddAttributes(new ezColorAttribute(ezColor::LightCoral)),
    EZ_MEMBER_PROPERTY("Message", m_sMessage),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSimpleRenderPass::ezSimpleRenderPass(const char* szName)
  : ezRenderPipelinePass(szName, true)
{
}

ezSimpleRenderPass::~ezSimpleRenderPass() {}

bool ezSimpleRenderPass::GetRenderTargetDescriptions(
  const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription* const> inputs, ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALRenderTargetSetup& setup = view.GetRenderTargetSetup();

  // Color
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }
  else
  {
    // If no input is available, we use the render target setup instead.
    const ezGALRenderTargetView* pTarget = pDevice->GetRenderTargetView(setup.GetRenderTarget(0));
    if (pTarget)
    {
      const ezGALRenderTargetViewCreationDescription& desc = pTarget->GetDescription();
      const ezGALTexture* pTexture = pDevice->GetTexture(desc.m_hTexture);
      if (pTexture)
      {
        outputs[m_PinColor.m_uiOutputIndex] = pTexture->GetDescription();
        outputs[m_PinColor.m_uiOutputIndex].m_bCreateRenderTarget = true;
        outputs[m_PinColor.m_uiOutputIndex].m_bAllowShaderResourceView = true;
        outputs[m_PinColor.m_uiOutputIndex].m_ResourceAccess.m_bReadBack = false;
        outputs[m_PinColor.m_uiOutputIndex].m_ResourceAccess.m_bImmutable = true;
        outputs[m_PinColor.m_uiOutputIndex].m_pExisitingNativeObject = nullptr;
      }
    }
  }

  // DepthStencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    // If no input is available, we use the render target setup instead.
    const ezGALRenderTargetView* pTarget = pDevice->GetRenderTargetView(setup.GetDepthStencilTarget());
    if (pTarget)
    {
      const ezGALRenderTargetViewCreationDescription& desc = pTarget->GetDescription();
      const ezGALTexture* pTexture = pDevice->GetTexture(desc.m_hTexture);
      if (pTexture)
      {
        outputs[m_PinDepthStencil.m_uiOutputIndex] = pTexture->GetDescription();
      }
    }
  }

  return true;
}

void ezSimpleRenderPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
  const ezArrayPtr<ezRenderPipelinePassConnection* const> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Setup render target
  ezGALRenderingSetup renderingSetup;
  if (inputs[m_PinColor.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinColor.m_uiInputIndex]->m_TextureHandle));
  }

  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
  }

  auto pCommandEncoder = ezRenderContext::BeginPassAndRenderingScope(renderViewContext, std::move(renderingSetup), GetName());

  // Setup Permutation Vars
  ezTempHashedString sRenderPass("RENDER_PASS_FORWARD");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != ezViewRenderMode::None)
  {
    sRenderPass = ezViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  // Execute render functions
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleOpaque);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleTransparent);

  if (!m_sMessage.IsEmpty())
  {
    ezDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, m_sMessage, ezVec2I32(20, 20), ezColor::OrangeRed);
  }

  ezDebugRenderer::Render(renderViewContext);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleForeground);

  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::GUI);
}

void ezSimpleRenderPass::SetMessage(const char* szMessage)
{
  m_sMessage = szMessage;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SimpleRenderPass);
