#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

struct PerInstanceData
{
	TRANSFORM(ObjectToWorld);
	TRANSFORM(ObjectToWorldNormal);
	INT1(GameObjectID);
	
	INT3(Reserved);
	FLOAT4(Reserved2);
};

#if INSTANCING

	#if EZ_ENABLED(PLATFORM_DX11)
		StructuredBuffer<PerInstanceData> perInstanceData;
	#endif
	
#else
	
	CONSTANT_BUFFER(PerInstanceConstants, 2) 
	{
		PerInstanceData perInstanceData;
	};
	
#endif



