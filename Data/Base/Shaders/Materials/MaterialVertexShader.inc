#pragma once

#define USE_WORLDPOS

#include <Shaders/Common/GlobalConstants.h>
#include <Shaders/Common/ObjectConstants.h>
#include <Shaders/Materials/MaterialInterpolator.inc>

PerInstanceData GetInstanceData(VS_IN Input)
{
	#if INSTANCING
		return perInstanceData[Input.InstanceID];
	#else
		return perInstanceData;
	#endif	
}

VS_OUT FillVertexData(VS_IN Input)
{
	PerInstanceData data = GetInstanceData(Input);
	float4x4 objectToWorld = TransformToMatrix(data.ObjectToWorld);
	float3x3 objectToWorldNormal = (float3x3)TransformToMatrix(data.ObjectToWorldNormal);
	
	VS_OUT Output;
	Output.WorldPosition = mul(objectToWorld, float4(Input.Position, 1.0)).xyz;
	Output.Position = mul(WorldToScreenMatrix, float4(Output.WorldPosition, 1.0));
	
	#if defined(USE_NORMAL)
		Output.Normal = normalize(mul(objectToWorldNormal, Input.Normal));
	#endif
	
	#if defined(USE_TANGENT)
		float handednessCorrection = 2.0f - dot(Input.Tangent, Input.Tangent);
		Output.Tangent = normalize(mul(objectToWorldNormal, Input.Tangent));
		Output.BiTangent = cross(Output.Normal, Output.Tangent) * handednessCorrection;
	#endif
	
	#if defined(USE_TEXCOORD0)
		#if USE_TEXCOORD1
			Output.TexCoords = float4(Input.TexCoord0, Input.TexCoord1);
		#else
			Output.TexCoords = Input.TexCoord0;
		#endif
	#endif
	
	#if defined(USE_COLOR)
		Output.Color = Input.Color;
	#endif
	
	#if INSTANCING
		Output.InstanceID = Input.InstanceID;
	#endif
	
	return Output;
}
