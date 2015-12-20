#include <PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <EnginePluginScene/EditorRenderPass/EditorRenderPass.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Meshes/MeshRenderer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorRenderPass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezEditorRenderPass::ezEditorRenderPass(ezSceneContext* pSceneContext, const ezGALRenderTagetSetup& RenderTargetSetup, const char* szName) : ezSimpleRenderPass(RenderTargetSetup, szName)
{
  m_pSceneContext = pSceneContext;
  m_bRenderSelectionOverlay = true;
  m_bRenderShapeIcons = true;
}

void ezEditorRenderPass::Execute(const ezRenderViewContext& renderViewContext)
{
  const char* szRenderMode = "ERM_DEFAULT";

  switch (m_ViewRenderMode)
  {
  case ezViewRenderMode::None:
    szRenderMode = "ERM_DEFAULT";
    break;
  case ezViewRenderMode::WireframeColor:
    szRenderMode = "ERM_WIREFRAME_COLOR";
    break;
  case ezViewRenderMode::WireframeMonochrome:
    szRenderMode = "ERM_WIREFRAME_MONOCHROME";
    break;
  case ezViewRenderMode::TexCoordsUV0:
    szRenderMode = "ERM_TEXCOORDS_UV0";
    break;
  case ezViewRenderMode::VertexNormals:
    szRenderMode = "ERM_VERTEX_NORMALS";
    break;
  case ezViewRenderMode::PixelDepth:
    szRenderMode = "ERM_PIXEL_DEPTH";
    break;
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", szRenderMode);

  // since typically the fov is tied to the height, we orient the gizmo size on that
  const float fGizmoScale = 128.0f / (float) renderViewContext.m_pViewData->m_ViewPortRect.height;
  ezRenderContext::GetDefaultInstance()->SetMaterialParameter("GizmoScale", fGizmoScale);

  //ezSimpleRenderPass::Execute(renderViewContext);
  {
    ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

    const ezRectFloat& viewPortRect = renderViewContext.m_pViewData->m_ViewPortRect;
    pGALContext->SetViewport(viewPortRect.x, viewPortRect.y, viewPortRect.width, viewPortRect.height, 0.0f, 1.0f);

    pGALContext->SetRenderTargetSetup(m_RenderTargetSetup);
    pGALContext->Clear(ezColor(0.0f, 0.0f, 0.1f));

    RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Opaque);
    RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Masked);
    RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Transparent);

    if (/*m_ViewRenderMode == ezViewRenderMode::Default && */m_bRenderSelectionOverlay)
    {
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_SELECTED");
      RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Selection);
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", szRenderMode);
    }

    m_pSceneContext->RenderShapeIcons(renderViewContext.m_pRenderContext);

    pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f), 0); // only clear depth

    RenderDataWithPassType(renderViewContext, ezDefaultPassTypes::Foreground);

  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_DEFAULT");
}
