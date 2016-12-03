#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

#define WIREFRAME_RENDER_PASS_MONOCHROME 1
#define WIREFRAME_RENDER_PASS_COLOR 2

#define EDITOR_RENDER_PASS_LIT_ONLY 1
#define	EDITOR_RENDER_PASS_TEXCOORDS_UV0 2
#define	EDITOR_RENDER_PASS_VERTEX_NORMALS 3
#define	EDITOR_RENDER_PASS_VERTEX_TANGENTS 4
#define	EDITOR_RENDER_PASS_PIXEL_NORMALS 5
#define	EDITOR_RENDER_PASS_DIFFUSE_COLOR 6
#define	EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE 7
#define	EDITOR_RENDER_PASS_SPECULAR_COLOR 8
#define	EDITOR_RENDER_PASS_EMISSIVE_COLOR 9
#define	EDITOR_RENDER_PASS_ROUGHNESS 10
#define	EDITOR_RENDER_PASS_OCCLUSION 11
#define	EDITOR_RENDER_PASS_DEPTH 12

CONSTANT_BUFFER(ezGlobalConstants, 0)
{
  FLOAT3(CameraPosition);
  FLOAT1(Padding1);

  FLOAT3(CameraDirForwards);
  FLOAT1(Padding2);

  FLOAT3(CameraDirRight);
  FLOAT1(Padding3);

  FLOAT3(CameraDirUp);
  FLOAT1(Padding4);

  MAT4(CameraToScreenMatrix);
  MAT4(ScreenToCameraMatrix);
  MAT4(WorldToCameraMatrix);
  MAT4(CameraToWorldMatrix);
  MAT4(WorldToScreenMatrix);
  MAT4(ScreenToWorldMatrix);
  FLOAT4(Viewport);

  FLOAT1(DeltaTime);
  FLOAT1(GlobalTime);
  FLOAT1(WorldTime);

  FLOAT1(Exposure);

  INT1(RenderPass);
  UINT1(NumMsaaSamples);
};


