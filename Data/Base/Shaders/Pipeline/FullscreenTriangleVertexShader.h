#pragma once

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Pipeline/FullscreenTriangleInterpolator.h>

VS_OUT main(uint vertexId : SV_VertexID, uint InstanceId : SV_InstanceID)
{
    VS_OUT Output;
    Output.Position.x = (float)(vertexId / 2) * 4.0f - 1.0f;
    Output.Position.y = (float)(vertexId % 2) * 4.0f - 1.0f;
    Output.Position.z = 0.0f;
    Output.Position.w = 1.0f;

    Output.TexCoord0.x = (float)(vertexId / 2) * 2.0f;
    Output.TexCoord0.y = 1.0f - (float)(vertexId % 2) * 2.0f;

#if CAMERA_MODE == CAMERA_MODE_STEREO
    Output.RenderTargetArrayIndex = InstanceId;
#endif

    return Output;
}
