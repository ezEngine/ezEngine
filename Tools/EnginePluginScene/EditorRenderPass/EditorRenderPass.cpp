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
    EZ_ENUM_MEMBER_PROPERTY("ViewRenderMode", ezViewRenderMode, m_ViewRenderMode)
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezEditorRenderPass::ezEditorRenderPass(const char* szName) : ezForwardRenderPass(szName)
{
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
  pGALContext->Clear(ezColor(0.01f, 0.01f, 0.01f));

  ezTempHashedString sRenderPass("FORWARD");
  ezUInt32 uiRenderPass = 0;

  switch (m_ViewRenderMode)
  {
  case ezViewRenderMode::WireframeColor:
    sRenderPass = "WIREFRAME";
    uiRenderPass = WIREFRAME_RENDER_PASS_COLOR;
    break;
  case ezViewRenderMode::WireframeMonochrome:
    sRenderPass = "WIREFRAME";
    uiRenderPass = WIREFRAME_RENDER_PASS_MONOCHROME;
    break;
  case ezViewRenderMode::TexCoordsUV0:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_TEXCOORDS_UV0;
    break;
  case ezViewRenderMode::VertexNormals:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_NORMALS;
    break;
  case ezViewRenderMode::PixelDepth:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_DEPTH;
    break;
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  auto& globalConstants = renderViewContext.m_pRenderContext->WriteGlobalConstants();
  globalConstants.RenderPass = uiRenderPass;

  //ezSimpleRenderPass::Execute(renderViewContext);
  {
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitOpaque);
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitMasked);
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitTransparent);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
    RenderDataWithCategory(renderViewContext, ezDefaultRenderDataCategories::LitForeground);
  }
}
