#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

#define	WIREFRAME_RENDER_PASS_PICKING 0
#define WIREFRAME_RENDER_PASS_MONOCHROME 1
#define WIREFRAME_RENDER_PASS_COLOR 2

#define	EDITOR_RENDER_PASS_PICKING 0
#define	EDITOR_RENDER_PASS_TEXCOORDS_UV0 1
#define	EDITOR_RENDER_PASS_NORMALS 2
#define	EDITOR_RENDER_PASS_DEPTH 3

CONSTANT_BUFFER(GlobalConstants, 0)
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
  
  INT1(RenderPass);
};


