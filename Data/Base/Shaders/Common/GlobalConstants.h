#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

#define WIREFRAME_RENDER_PASS_MONOCHROME 1
#define WIREFRAME_RENDER_PASS_COLOR 2

#define EDITOR_RENDER_PASS_LIT_ONLY 1
#define	EDITOR_RENDER_PASS_TEXCOORDS_UV0 2
#define	EDITOR_RENDER_PASS_NORMALS 3
#define	EDITOR_RENDER_PASS_DIFFUSE_COLOR 4
#define	EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE 5
#define	EDITOR_RENDER_PASS_SPECULAR_COLOR 6
#define	EDITOR_RENDER_PASS_ROUGHNESS 7
#define	EDITOR_RENDER_PASS_DEPTH 8

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

  FLOAT1(GlobalTime);
  FLOAT1(WorldTime);

  INT1(RenderPass);
  UINT1(NumMsaaSamples);
};


