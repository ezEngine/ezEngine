#pragma once

#include <Shaders/Common/GlobalConstants.h>

struct VS_OUT
{
	float4 Position : SV_Position;
	float2 TexCoords : TEXCOORD0;
};

VS_OUT main(uint vertexId : SV_VERTEXID)
{
	VS_OUT Output;
	Output.Position.x = (float)(vertexId / 2) * 4.0f - 1.0f;
	Output.Position.y = (float)(vertexId % 2) * 4.0f - 1.0f;
	Output.Position.z = 0.0f;
	Output.Position.w = 1.0f;
	
	Output.TexCoords.x = (float)(vertexId / 2) * 2.0f;
	Output.TexCoords.y = 1.0f - (float)(vertexId % 2) * 2.0f;
	
	return Output;
}
