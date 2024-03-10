#pragma once

#include "ConstantBufferMacros.h"
#include "Platforms.h"

#define MIN_PERCEPTUAL_ROUGHNESS 0.045
#define MIN_ROUGHNESS 0.002025

#define WIREFRAME_RENDER_PASS_COLOR 1
#define WIREFRAME_RENDER_PASS_MONOCHROME 2

#define EDITOR_RENDER_PASS_DIFFUSE_LIT_ONLY 1
#define EDITOR_RENDER_PASS_SPECULAR_LIT_ONLY 2
#define EDITOR_RENDER_PASS_LIGHT_COUNT 3
#define EDITOR_RENDER_PASS_DECAL_COUNT 4
#define EDITOR_RENDER_PASS_TEXCOORDS_UV0 5
#define EDITOR_RENDER_PASS_TEXCOORDS_UV1 6
#define EDITOR_RENDER_PASS_VERTEX_COLORS0 7
#define EDITOR_RENDER_PASS_VERTEX_COLORS1 8
#define EDITOR_RENDER_PASS_VERTEX_NORMALS 9
#define EDITOR_RENDER_PASS_VERTEX_TANGENTS 10
#define EDITOR_RENDER_PASS_PIXEL_NORMALS 11
#define EDITOR_RENDER_PASS_DIFFUSE_COLOR 12
#define EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE 13
#define EDITOR_RENDER_PASS_SPECULAR_COLOR 14
#define EDITOR_RENDER_PASS_EMISSIVE_COLOR 15
#define EDITOR_RENDER_PASS_ROUGHNESS 16
#define EDITOR_RENDER_PASS_OCCLUSION 17
#define EDITOR_RENDER_PASS_DEPTH 18
#define EDITOR_RENDER_PASS_STATIC_VS_DYNAMIC 19
#define EDITOR_RENDER_PASS_BONE_WEIGHTS 20

CONSTANT_BUFFER(ezGlobalConstants, 0)
{
  // Use functions from CameraConstantsAccess.h to access these and derived camera properties.
  MAT4(CameraToScreenMatrix)
  [2];
  MAT4(ScreenToCameraMatrix)
  [2];
  MAT4(WorldToCameraMatrix)
  [2];
  MAT4(CameraToWorldMatrix)
  [2];
  MAT4(WorldToScreenMatrix)
  [2];
  MAT4(ScreenToWorldMatrix)
  [2];

  FLOAT4(ViewportSize); // x = width, y = height, z = 1 / width, w = 1 / height
  FLOAT4(ClipPlanes);   // x = near, y = far, z = 1 / far
  FLOAT1(MaxZValue);    // any screenspace z values smaller than this value are clamped. Used for directional shadows.

  FLOAT1(DeltaTime);
  FLOAT1(GlobalTime);
  FLOAT1(WorldTime);

  FLOAT1(Exposure);

  INT1(RenderPass);
  UINT1(NumMsaaSamples);
};

#include "CameraConstantsAccess.h"
