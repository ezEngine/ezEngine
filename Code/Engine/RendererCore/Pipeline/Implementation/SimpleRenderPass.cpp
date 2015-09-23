#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Meshes/MeshRenderer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimpleRenderPass, ezRenderPipelinePass, 1, ezRTTIDefaultAllocator<ezSimpleRenderPass>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Color", m_PinColor),
    EZ_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil)
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSimpleRenderPass::ezSimpleRenderPass(const char* szName) : ezRenderPipelinePass(szName)
{

}

ezSimpleRenderPass::ezSimpleRenderPass(const ezGALRenderTagetSetup& RenderTargetSetup, const char* szName) : ezRenderPipelinePass(szName)
{
  m_RenderTargetSetup = RenderTargetSetup;
  AddRenderer( EZ_DEFAULT_NEW( ezMeshRenderer ) );
}

ezSimpleRenderPass::~ezSimpleRenderPass()
{

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

  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Foreground);
}
