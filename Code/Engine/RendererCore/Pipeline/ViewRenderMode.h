#pragma once

#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Pipeline/Declarations.h>

/// Defines different visualization modes for a view.
///
/// Used for debugging and inspecting various rendering aspects like wireframe, normals,
/// texture coordinates, lighting components, etc. The render mode affects shader selection
/// through permutation variables.
struct EZ_RENDERERCORE_DLL ezViewRenderMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None,                ///< Normal rendering
    WireframeColor,      ///< Wireframe with per-object colors
    WireframeMonochrome, ///< Monochrome wireframe
    DiffuseLitOnly,      ///< Show only diffuse lighting
    SpecularLitOnly,     ///< Show only specular lighting
    LightCount,          ///< Visualize number of lights in the cluster affecting each pixel
    DecalCount,          ///< Visualize number of decals in the cluster affecting each pixel
    TexCoordsUV0,        ///< Show UV0 texture coordinates
    TexCoordsUV1,        ///< Show UV1 texture coordinates
    VertexColors0,       ///< Show vertex color channel 0
    VertexColors1,       ///< Show vertex color channel 1
    VertexNormals,       ///< Show vertex normals
    VertexTangents,      ///< Show vertex tangents
    PixelNormals,        ///< Show per-pixel normals
    DiffuseColor,        ///< Show diffuse albedo
    DiffuseColorRange,   ///< Show diffuse albedo with range visualization
    SpecularColor,       ///< Show specular color
    EmissiveColor,       ///< Show emissive color
    Roughness,           ///< Show material roughness
    Occlusion,           ///< Show ambient occlusion
    Depth,               ///< Show depth buffer
    StaticVsDynamic,     ///< Visualize static vs dynamic objects
    BoneWeights,         ///< Visualize skeletal animation bone weights

    ENUM_COUNT,

    Default = None
  };

  /// Returns the shader permutation variable value for the given render mode.
  static ezTempHashedString GetPermutationValue(Enum renderMode);

  /// Returns which render pass should be used for the given render mode.
  static int GetRenderPassForShader(Enum renderMode);

  /// Returns a debug text description for the given render mode.
  static void GetDebugText(Enum renderMode, ezStringBuilder& out_sDebugText);
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezViewRenderMode);
