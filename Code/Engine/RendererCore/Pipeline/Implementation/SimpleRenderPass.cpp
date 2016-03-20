#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Lights/LightGatheringRenderer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimpleRenderPass, 1, ezRTTIDefaultAllocator<ezSimpleRenderPass>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil)
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

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
    ezGALRenderTargetViewCreationDescription rtvd;
    rtvd.m_hTexture = outputs[m_PinColor.m_uiOutputIndex]->m_TextureHandle;
    rtvd.m_RenderTargetType = ezGALRenderTargetType::Color;

    renderTargetSetup.SetRenderTarget(0, pDevice->CreateRenderTargetView(rtvd));
  }

  if (outputs[m_PinDepthStencil.m_uiOutputIndex])
  {
    ezGALRenderTargetViewCreationDescription rtvd;
    rtvd.m_hTexture = outputs[m_PinDepthStencil.m_uiOutputIndex]->m_TextureHandle;
    rtvd.m_RenderTargetType = ezGALRenderTargetType::DepthStencil;

    renderTargetSetup.SetDepthStencilTarget(pDevice->CreateRenderTargetView(rtvd));
  }

  // Execute render functions
  const ezRectFloat& viewPortRect = renderViewContext.m_pViewData->m_ViewPortRect;
  pGALContext->SetViewport(viewPortRect.x, viewPortRect.y, viewPortRect.width, viewPortRect.height, 0.0f, 1.0f);

  pGALContext->SetRenderTargetSetup(renderTargetSetup);
  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.1f));

  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Opaque);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Masked);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Transparent);

  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f), 0); // only clear depth

  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Foreground1);
  RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Foreground2);
}
