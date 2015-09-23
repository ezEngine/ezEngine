#include <PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <EnginePluginScene/EditorRenderPass/EditorRenderPass.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Meshes/MeshRenderer.h>

ezEditorRenderPass::ezEditorRenderPass(const ezGALRenderTagetSetup& RenderTargetSetup, const char* szName) : ezSimpleRenderPass(RenderTargetSetup, szName)
{
}

void ezEditorRenderPass::Execute(const ezRenderViewContext& renderViewContext)
{
  switch (m_ViewRenderMode)
  {
  case ezViewRenderMode::WireframeColor:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_WIREFRAME", "1");
    break;
  case ezViewRenderMode::WireframeMonochrome:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_WIREFRAME", "2");
    break;
  }

  ezSimpleRenderPass::Execute(renderViewContext);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_WIREFRAME", "0");
}
