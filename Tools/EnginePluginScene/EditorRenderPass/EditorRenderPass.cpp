#include <PCH.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <EnginePluginScene/EditorRenderPass/EditorRenderPass.h>

#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Debug/DebugRenderer.h>

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

  ezStringBuilder sDebugText;

  switch (m_ViewRenderMode)
  {
  case ezViewRenderMode::None:
    break;
  case ezViewRenderMode::WireframeColor:
    sRenderPass = "WIREFRAME";
    uiRenderPass = WIREFRAME_RENDER_PASS_COLOR;
    break;
  case ezViewRenderMode::WireframeMonochrome:
    sRenderPass = "WIREFRAME";
    uiRenderPass = WIREFRAME_RENDER_PASS_MONOCHROME;
    break;
  case ezViewRenderMode::LitOnly:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_LIT_ONLY;
    break;
  case ezViewRenderMode::TexCoordsUV0:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_TEXCOORDS_UV0;
    break;
  case ezViewRenderMode::PixelNormals:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_PIXEL_NORMALS;
    break;
  case ezViewRenderMode::VertexNormals:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_VERTEX_NORMALS;
    break;
  case ezViewRenderMode::VertexTangents:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_VERTEX_TANGENTS;
    break;
  case ezViewRenderMode::DiffuseColor:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_DIFFUSE_COLOR;
    break;
  case ezViewRenderMode::DiffuseColorRange:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE;
    sDebugText = "Pure magenta means the diffuse color is too dark, pure green means it is too bright.";
    break;
  case ezViewRenderMode::SpecularColor:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_SPECULAR_COLOR;
    break;
  case ezViewRenderMode::EmissiveColor:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_EMISSIVE_COLOR;
    break;
  case ezViewRenderMode::Roughness:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_ROUGHNESS;
    break;
  case ezViewRenderMode::Occlusion:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_OCCLUSION;
    break;
  case ezViewRenderMode::Depth:
    sRenderPass = "EDITOR";
    uiRenderPass = EDITOR_RENDER_PASS_DEPTH;
    break;
  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  auto& globalConstants = renderViewContext.m_pRenderContext->WriteGlobalConstants();
  globalConstants.RenderPass = uiRenderPass;

  if (!sDebugText.IsEmpty())
  {
    ezDebugRenderer::DrawText(*renderViewContext.m_pViewDebugContext, sDebugText, ezVec2I32(10, 10), ezColor::White);
  }
}
