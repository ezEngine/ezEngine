#pragma once

#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/Declarations.h>

struct EZ_RENDERERCORE_DLL ezViewRenderMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None,
    WireframeColor,
    WireframeMonochrome,
    DiffuseLitOnly,
    SpecularLitOnly,
    LightCount,
    DecalCount,
    TexCoordsUV0,
    TexCoordsUV1,
    VertexColors0,
    VertexColors1,
    VertexNormals,
    VertexTangents,
    PixelNormals,
    DiffuseColor,
    DiffuseColorRange,
    SpecularColor,
    EmissiveColor,
    Roughness,
    Occlusion,
    Depth,
    StaticVsDynamic,
    BoneWeights,

    ENUM_COUNT,

    Default = None
  };

  static ezTempHashedString GetPermutationValue(Enum renderMode);
  static int GetRenderPassForShader(Enum renderMode);
  static void GetDebugText(Enum renderMode, ezStringBuilder& out_sDebugText);
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezViewRenderMode);
