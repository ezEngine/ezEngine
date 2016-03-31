#include <RendererCore/PCH.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/DebugRenderPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/Meshes/MeshRenderer.h>
#include <RendererCore/Lights/LightGatheringRenderer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDebugRenderPass, 1, ezRTTIDefaultAllocator<ezDebugRenderPass>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil)
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezDebugRenderPass::ezDebugRenderPass(const char* szName) : ezRenderPipelinePass(szName)
{
}

ezDebugRenderPass::~ezDebugRenderPass()
{
}

bool ezDebugRenderPass::GetRenderTargetDescriptions(const ezView& view, const ezArrayPtr<ezGALTextureCreationDescription*const> inputs,
  ezArrayPtr<ezGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinColor.m_uiInputIndex] != nullptr)
  {
    outputs[m_PinColor.m_uiOutputIndex] = *inputs[m_PinColor.m_uiInputIndex];
  }
  else
  {
    // always needs an input
    return false;
  }
  
  // DepthStencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex] != nullptr)
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    // always needs an input
    return false;
  }

  return true;
}

void ezDebugRenderPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
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
  
  ezDebugRenderer::Render(renderViewContext);
}
