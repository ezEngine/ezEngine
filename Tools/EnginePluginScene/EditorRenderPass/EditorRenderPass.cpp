#include <PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <EnginePluginScene/EditorRenderPass/EditorRenderPass.h>
#include <EnginePluginScene/SceneContext/SceneContext.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Meshes/MeshRenderer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEditorRenderPass, 1, ezRTTIDefaultAllocator<ezEditorRenderPass>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("RenderSelectionOverlay", m_bRenderSelectionOverlay),
EZ_ENUM_MEMBER_PROPERTY("ViewRenderMode", ezViewRenderMode, m_ViewRenderMode),
EZ_MEMBER_PROPERTY("SceneContext", m_pSceneContext),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezEditorRenderPass::ezEditorRenderPass(const char* szName) : ezSimpleRenderPass(szName)
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

    pGALContext->SetRenderTargetSetup(renderTargetSetup);
    pGALContext->Clear(ezColor(0.0f, 0.0f, 0.1f));

    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Opaque);
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Masked);
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Transparent);

    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Selection);

    if (/*m_ViewRenderMode == ezViewRenderMode::Default && */m_bRenderSelectionOverlay)
    {
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_SELECTED");
      RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Selection);
      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", szRenderMode);
    }

    pGALContext->Clear(ezColor(0.0f, 0.0f, 0.0f, 0.0f), 0); // only clear depth

    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Foreground1);
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::Foreground2);

  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("EDITOR_RENDER_MODE", "ERM_DEFAULT");
}
