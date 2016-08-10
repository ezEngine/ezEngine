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

void ezEditorRenderPass::SetupPermutationVars(const ezRenderViewContext& renderViewContext)
{
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
}
