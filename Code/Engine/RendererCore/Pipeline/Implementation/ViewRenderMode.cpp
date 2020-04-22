#include <RendererCorePCH.h>

#include <RendererCore/Pipeline/ViewRenderMode.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/GlobalConstants.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezViewRenderMode, 1)
  EZ_ENUM_CONSTANT(ezViewRenderMode::None)->AddAttributes(new ezGroupAttribute("Default")),
  EZ_ENUM_CONSTANT(ezViewRenderMode::WireframeColor)->AddAttributes(new ezGroupAttribute("Wireframe")),
  EZ_ENUM_CONSTANT(ezViewRenderMode::WireframeMonochrome),
  EZ_ENUM_CONSTANT(ezViewRenderMode::LitOnly)->AddAttributes(new ezGroupAttribute("Lighting")),
  EZ_ENUM_CONSTANT(ezViewRenderMode::LightCount)->AddAttributes(new ezGroupAttribute("Performance")),
  EZ_ENUM_CONSTANT(ezViewRenderMode::DecalCount),
  EZ_ENUM_CONSTANT(ezViewRenderMode::StaticVsDynamic),
  EZ_ENUM_CONSTANT(ezViewRenderMode::TexCoordsUV0)->AddAttributes(new ezGroupAttribute("TexCoords")),
  EZ_ENUM_CONSTANT(ezViewRenderMode::TexCoordsUV1),
  EZ_ENUM_CONSTANT(ezViewRenderMode::VertexColors0)->AddAttributes(new ezGroupAttribute("VertexColors")),
  EZ_ENUM_CONSTANT(ezViewRenderMode::VertexColors1),
  EZ_ENUM_CONSTANT(ezViewRenderMode::VertexNormals)->AddAttributes(new ezGroupAttribute("Normals")),
  EZ_ENUM_CONSTANT(ezViewRenderMode::VertexTangents),
  EZ_ENUM_CONSTANT(ezViewRenderMode::PixelNormals),
  EZ_ENUM_CONSTANT(ezViewRenderMode::DiffuseColor)->AddAttributes(new ezGroupAttribute("PixelColors")),
  EZ_ENUM_CONSTANT(ezViewRenderMode::DiffuseColorRange),
  EZ_ENUM_CONSTANT(ezViewRenderMode::SpecularColor),
  EZ_ENUM_CONSTANT(ezViewRenderMode::EmissiveColor),
  EZ_ENUM_CONSTANT(ezViewRenderMode::Roughness)->AddAttributes(new ezGroupAttribute("Surface")),
  EZ_ENUM_CONSTANT(ezViewRenderMode::Occlusion),
  EZ_ENUM_CONSTANT(ezViewRenderMode::Depth),
  EZ_ENUM_CONSTANT(ezViewRenderMode::BoneWeights)->AddAttributes(new ezGroupAttribute("Animation")),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
ezTempHashedString ezViewRenderMode::GetPermutationValue(Enum renderMode)
{
  if (renderMode >= WireframeColor && renderMode <= WireframeMonochrome)
  {
    return "RENDER_PASS_WIREFRAME";
  }
  else if (renderMode >= LitOnly && renderMode < ENUM_COUNT)
  {
    return "RENDER_PASS_EDITOR";
  }

  return "";
}

// static
int ezViewRenderMode::GetRenderPassForShader(Enum renderMode)
{
  switch (renderMode)
  {
    case ezViewRenderMode::None:
      return -1;

    case ezViewRenderMode::WireframeColor:
      return WIREFRAME_RENDER_PASS_COLOR;

    case ezViewRenderMode::WireframeMonochrome:
      return WIREFRAME_RENDER_PASS_MONOCHROME;

    case ezViewRenderMode::LitOnly:
      return EDITOR_RENDER_PASS_LIT_ONLY;

    case ezViewRenderMode::LightCount:
      return EDITOR_RENDER_PASS_LIGHT_COUNT;

    case ezViewRenderMode::DecalCount:
      return EDITOR_RENDER_PASS_DECAL_COUNT;

    case ezViewRenderMode::TexCoordsUV0:
      return EDITOR_RENDER_PASS_TEXCOORDS_UV0;

    case ezViewRenderMode::TexCoordsUV1:
      return EDITOR_RENDER_PASS_TEXCOORDS_UV1;

    case ezViewRenderMode::VertexColors0:
      return EDITOR_RENDER_PASS_VERTEX_COLORS0;

    case ezViewRenderMode::VertexColors1:
      return EDITOR_RENDER_PASS_VERTEX_COLORS1;

    case ezViewRenderMode::VertexNormals:
      return EDITOR_RENDER_PASS_VERTEX_NORMALS;

    case ezViewRenderMode::VertexTangents:
      return EDITOR_RENDER_PASS_VERTEX_TANGENTS;

    case ezViewRenderMode::PixelNormals:
      return EDITOR_RENDER_PASS_PIXEL_NORMALS;

    case ezViewRenderMode::DiffuseColor:
      return EDITOR_RENDER_PASS_DIFFUSE_COLOR;

    case ezViewRenderMode::DiffuseColorRange:
      return EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE;

    case ezViewRenderMode::SpecularColor:
      return EDITOR_RENDER_PASS_SPECULAR_COLOR;

    case ezViewRenderMode::EmissiveColor:
      return EDITOR_RENDER_PASS_EMISSIVE_COLOR;

    case ezViewRenderMode::Roughness:
      return EDITOR_RENDER_PASS_ROUGHNESS;

    case ezViewRenderMode::Occlusion:
      return EDITOR_RENDER_PASS_OCCLUSION;

    case ezViewRenderMode::Depth:
      return EDITOR_RENDER_PASS_DEPTH;

    case ezViewRenderMode::StaticVsDynamic:
      return EDITOR_RENDER_PASS_STATIC_VS_DYNAMIC;

    case ezViewRenderMode::BoneWeights:
      return EDITOR_RENDER_PASS_BONE_WEIGHTS;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return -1;
  }
}

// static
void ezViewRenderMode::GetDebugText(Enum renderMode, ezStringBuilder& out_sDebugText)
{
  if (renderMode == DiffuseColorRange)
  {
    out_sDebugText = "Pure magenta means the diffuse color is too dark, pure green means it is too bright.";
  }
  else if (renderMode == StaticVsDynamic)
  {
    out_sDebugText = "Static objects are shown in green, dynamic objects are shown in red.";
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_ViewRenderMode);

