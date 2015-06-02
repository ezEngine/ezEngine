#include <RendererCore/PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/SimpleRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Meshes/MeshRenderer.h>

ezSimpleRenderPass::ezSimpleRenderPass(ezGALRenderTargetConfigHandle hRTConfig) : ezRenderPipelinePass("SimpleRenderPass")
{
  m_hRTConfig = hRTConfig;
  AddRenderer(EZ_DEFAULT_NEW(ezMeshRenderer));
}

ezSimpleRenderPass::~ezSimpleRenderPass()
{

}

void ezSimpleRenderPass::Execute(const ezRenderViewContext& renderViewContext)
{
  ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

  pGALContext->SetRenderTargetConfig(m_hRTConfig);
  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.1f));

  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Opaque);
  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Masked);
  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Transparent);

  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f), 0); // only clear depth

  RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Foreground);
}
