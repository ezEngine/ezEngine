#pragma once

#include <RendererCore/Pipeline/Declarations.h>

struct EZ_RENDERERCORE_DLL ezViewRenderMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None,
    WireframeColor,
    WireframeMonochrome,
    LitOnly,
    LightCount,
    DecalCount,
    TexCoordsUV0,
    PixelNormals,
    VertexNormals,
    VertexTangents,
    DiffuseColor,
    DiffuseColorRange,
    SpecularColor,
    EmissiveColor,
    Roughness,
    Occlusion,
    Depth,
    StaticVsDynamic,

    Default = None
  };

  static ezTempHashedString GetRenderPassPermutationValue(ezViewRenderMode::Enum renderMode);
  static int GetRenderPassForShader(ezViewRenderMode::Enum renderMode);
  static void GetDebugText(ezViewRenderMode::Enum renderMode, ezStringBuilder& out_sDebugText);
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezViewRenderMode);
