#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

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
  AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
  AddRenderer(EZ_DEFAULT_NEW(ezLightGatheringRenderer));
}

ezSimpleRenderPass::~ezSimpleRenderPass()
{
  m_RenderTargetSetup.DestroyAllAttachedViews();
}

bool ezSimpleRenderPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs,
  ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  const ezGALRenderTagetSetup& setup = view.GetRenderTargetSetup();

  // Color
  const ezNodePin* pColor = GetPinByName("Color");  
  if (inputs[pColor->m_uiInputIndex])
  {
    outputs[pColor->m_uiOutputIndex] = *inputs[pColor->m_uiInputIndex];
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
        outputs[pColor->m_uiOutputIndex] = pTexture->GetDescription();
      }
    }
  }
  
  // DepthStencil
  const ezNodePin* pDepth = GetPinByName("DepthStencil");
  if (inputs[pDepth->m_uiInputIndex])
  {
    outputs[pDepth->m_uiOutputIndex] = *inputs[pDepth->m_uiInputIndex];
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
        outputs[pDepth->m_uiOutputIndex] = pTexture->GetDescription();
      }
    }
  }

  return true;
}

void ezSimpleRenderPass::SetRenderTargets(const ezArrayPtr<ezRenderPipelinePassConnection*const> inputs, const ezArrayPtr<ezRenderPipelinePassConnection*const> outputs)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  m_RenderTargetSetup.DestroyAllAttachedViews();
  const ezNodePin* pColor = GetPinByName("Color");
  if (outputs[pColor->m_uiOutputIndex])
  {
    ezGALRenderTargetViewCreationDescription rtvd;
    rtvd.m_hTexture = outputs[pColor->m_uiOutputIndex]->m_TextureHandle;
    rtvd.m_RenderTargetType = ezGALRenderTargetType::Color;

    m_RenderTargetSetup.SetRenderTarget(0, pDevice->CreateRenderTargetView(rtvd));
  }

  const ezNodePin* pDepth = GetPinByName("DepthStencil");
  if (outputs[pDepth->m_uiOutputIndex])
  {
    ezGALRenderTargetViewCreationDescription rtvd;
    rtvd.m_hTexture = outputs[pDepth->m_uiOutputIndex]->m_TextureHandle;
    rtvd.m_RenderTargetType = ezGALRenderTargetType::DepthStencil;

    m_RenderTargetSetup.SetDepthStencilTarget(pDevice->CreateRenderTargetView(rtvd));
  }
}

void ezSimpleRenderPass::Execute(const ezRenderViewContext& renderViewContext)
{
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  const ezRectFloat& viewPortRect = renderViewContext.m_pViewData->m_ViewPortRect;
  pGALContext->SetViewport(viewPortRect.x, viewPortRect.y, viewPortRect.width, viewPortRect.height, 0.0f, 1.0f);

  pGALContext->SetRenderTargetSetup(m_RenderTargetSetup);
  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.1f));

  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Opaque);
  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Masked);
  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Transparent);

  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f), 0); // only clear depth

  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Foreground1);
  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Foreground2);
}
