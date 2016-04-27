#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/Passes/SimpleRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/Debug/DebugRenderer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimpleRenderPass, 1, ezRTTIDefaultAllocator<ezSimpleRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSimpleRenderPass::ezSimpleRenderPass(const char* szName) : ezRenderPipelinePass(szName)
{
}

ezSimpleRenderPass::~ezSimpleRenderPass()
{
}

bool ezSimpleRenderPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs,
  ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALRenderTagetSetup& setup = view.GetRenderTargetSetup();

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
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  // Setup render target
  ezGALRenderTagetSetup renderTargetSetup;
  if (outputs[m_PinColor.m_uiOutputIndex])
  {
    renderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinColor.m_uiOutputIndex]->m_TextureHandle));
  }

  if (outputs[m_PinDepthStencil.m_uiOutputIndex])
  {
    renderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(outputs[m_PinDepthStencil.m_uiOutputIndex]->m_TextureHandle));
  }

  pGALContext->SetRenderTargetSetup(renderTargetSetup);
  pGALContext->SetViewport(renderViewContext.m_pViewData->m_ViewPortRect);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "FORWARD");

  // Execute render functions
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleOpaque);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleTransparent);

  ezDebugRenderer::Render(renderViewContext);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::SimpleForeground);
}
