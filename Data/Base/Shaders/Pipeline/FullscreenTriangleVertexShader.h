#pragma once

#include <Shaders/Common/GlobalConstants.h>

struct VS_OUT
{
    float4 Position : SV_Position;
    float2 TexCoords : TEXCOORD0;
#if CAMERA_MODE == CAMERA_MODE_STEREO
    // TODO: In DX11.2 with "VPAndRTArrayIndexFromAnyShaderFeedingRasterizer" enabled, this could be the render output index already.
    // However, not all Desktop graphics cards support it which means that we need to rely on the Geometry shader :(
    // Once we support this feature, change the semantic to SV_RenderTargetArrayIndex
    uint RenderTargetArrayIndex : RENDERTARGETARRAYINDEX;
#endif
};

VS_OUT main(uint vertexId : SV_VertexID, uint InstanceId : SV_InstanceID)
{
    VS_OUT Output;
    Output.Position.x = (float)(vertexId / 2) * 4.0f - 1.0f;
    Output.Position.y = (float)(vertexId % 2) * 4.0f - 1.0f;
    Output.Position.z = 0.0f;
    Output.Position.w = 1.0f;
    
    Output.TexCoords.x = (float)(vertexId / 2) * 2.0f;
    Output.TexCoords.y = 1.0f - (float)(vertexId % 2) * 2.0f;
    
#if CAMERA_MODE == CAMERA_MODE_STEREO
    Output.RenderTargetArrayIndex = InstanceId;
#endif

    return Output;
}
