#include <PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <EnginePluginScene/EditorRenderPass/EditorRenderPass.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Meshes/MeshRenderer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorRenderPass, 1, ezRTTIDefaultAllocator<ezEditorRenderPass>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RenderSelectionOverlay", m_bRenderSelectionOverlay),
    EZ_ENUM_MEMBER_PROPERTY("ViewRenderMode", ezViewRenderMode, m_ViewRenderMode),
    EZ_MEMBER_PROPERTY("SceneContext", m_pSceneContext),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezEditorRenderPass::ezEditorRenderPass(const char* szName) : ezForwardRenderPass(szName)
{
  m_pSceneContext = nullptr;
  m_bRenderSelectionOverlay = true;
}

void ezEditorRenderPass::Execute(const ezRenderViewContext& renderViewContext, const ezArrayPtr<ezRenderPipelinePassConnection* const> inputs,
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

  // Clear color and depth stencil
  pGALContext->Clear(ezColor(0.0f, 0.0f, 0.1f));

  const char* szRenderMode = "DEFAULT";

  switch (m_ViewRenderMode)
  {
  case ezViewRenderMode::None:
    szRenderMode = "DEFAULT";
    break;
  case ezViewRenderMode::WireframeColor:
    szRenderMode = "WIREFRAME_COLOR";
    break;
  case ezViewRenderMode::WireframeMonochrome:
    szRenderMode = "WIREFRAME_MONOCHROME";
    break;
  case ezViewRenderMode::TexCoordsUV0:
    szRenderMode = "TEXCOORDS_UV0";
    break;
  case ezViewRenderMode::VertexNormals:
    szRenderMode = "VERTEX_NORMALS";
    break;
  case ezViewRenderMode::PixelDepth:
    szRenderMode = "PIXEL_DEPTH";
    break;
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", szRenderMode);

  // since typically the fov is tied to the height, we orient the gizmo size on that
  const float fGizmoScale = 128.0f / (float) renderViewContext.m_pViewData->m_ViewPortRect.height;
  ezRenderContext::GetDefaultInstance()->SetMaterialParameter("GizmoScale", fGizmoScale);

  //ezSimpleRenderPass::Execute(renderViewContext);
  {
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaque);
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMasked);
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitTransparent);

    if (/*m_ViewRenderMode == ezViewRenderMode::Default && */m_bRenderSelectionOverlay)
    {
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "SELECTED");
      RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Selection);
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", szRenderMode);
    }

    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);

  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "DEFAULT");
}
