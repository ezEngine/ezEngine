#pragma once

// This geometry shader is a pass-through that leaves the geometry unmodified and sets the render target array index.

#if CAMERA_MODE == CAMERA_MODE_STEREO && !defined(VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX)

#  include "MaterialInterpolator.h"

#  if defined(TOPOLOGY)
#    if TOPOLOGY == TOPOLOGY_LINES
[maxvertexcount(2)] void main(line VS_OUT input[2], inout LineStream<GS_OUT> outStream) {
  GS_OUT output;
  [unroll(3)] for (int i = 0; i < 2; ++i)
#    else
[maxvertexcount(3)] void main(triangle VS_OUT input[3], inout TriangleStream<GS_OUT> outStream) {
  GS_OUT output;
  [unroll(3)] for (int i = 0; i < 3; ++i)
#    endif
#  else
[maxvertexcount(3)] void main(triangle VS_OUT input[3], inout TriangleStream<GS_OUT> outStream) {
  GS_OUT output;
  [unroll(3)] for (int i = 0; i < 3; ++i)
#  endif

  {
    output.Position = input[i].Position;

#  if defined(USE_WORLDPOS)
    output.WorldPosition = input[i].WorldPosition;
#  endif

#  if defined(USE_NORMAL)
    output.Normal = input[i].Normal;
#  endif

#  if defined(USE_TANGENT)
    output.Tangent = input[i].Tangent;
    output.BiTangent = input[i].BiTangent;
#  endif

#  if defined(USE_TEXCOORD0)
    output.TexCoord0 = input[i].TexCoord0;
#  endif

#  if defined(USE_TEXCOORD1)
    output.TexCoord1 = input[i].TexCoord1;
#  endif

#  if defined(USE_COLOR0)
    output.Color0 = input[i].Color0;
#  endif

#  if defined(USE_COLOR1)
    output.Color1 = input[i].Color1;
#  endif

#  if defined(USE_DEBUG_INTERPOLATOR)
    output.DebugInterpolator = input[i].DebugInterpolator;
#  endif

#  if defined(CUSTOM_INTERPOLATOR)
    CopyCustomInterpolators(output, input);
#  endif

    output.InstanceID = input[i].InstanceID;
    output.RenderTargetArrayIndex = input[i].InstanceID % 2;

#  if defined(TWO_SIDED)
#    if TWO_SIDED == TRUE
    output.FrontFace : input[i].FrontFace;
#    endif
#  endif

    outStream.Append(output);
  }
}

#endif
