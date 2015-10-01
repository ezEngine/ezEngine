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
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_WIREFRAME_COLOR");
    break;
  case ezViewRenderMode::WireframeMonochrome:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_WIREFRAME_MONOCHROME");
    break;
  case ezViewRenderMode::TexCoordsUV0:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_TEXCOORDS_UV0");
    break;
  case ezViewRenderMode::VertexNormals:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_VERTEX_NORMALS");
    break;
  case ezViewRenderMode::PixelDepth:
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_PIXEL_DEPTH");
    break;
  }

  // since typically the fov is tied to the height, we orient the gizmo size on that
  const float fGizmoScale = 128.0f / (float) renderViewContext.m_pViewData->m_ViewPortRect.height;
  ezRenderContext::GetDefaultInstance()->SetMaterialParameter("GizmoScale", fGizmoScale);

  ezSimpleRenderPass::Execute(renderViewContext);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_DEFAULT");
}
