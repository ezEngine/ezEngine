#pragma once

// This geometry shader is a pass-through that leaves the geometry unmodified and sets the render target array index.

#if CAMERA_MODE == CAMERA_MODE_STEREO

struct GEOM_IN
{
  float4 Position : SV_Position;
  float2 TexCoord0 : TEXCOORD0;
  uint RenderTargetArrayIndex  : RENDERTARGETARRAYINDEX;
};
struct GEOM_OUT
{
  float4 Position : SV_Position;
  float2 TexCoord0 : TEXCOORD0;
  uint RenderTargetArrayIndex : SV_RenderTargetArrayIndex;
};

[maxvertexcount(3)]
void main(triangle GEOM_IN input[3], inout TriangleStream<GEOM_OUT> outStream)
{
    GEOM_OUT output;
    [unroll(3)] for (int i = 0; i < 3; ++i)
    {
        output.Position = input[i].Position;
        output.TexCoord0 = input[i].TexCoord0;
        output.RenderTargetArrayIndex = input[i].RenderTargetArrayIndex;
        outStream.Append(output);
    }
}

#endif