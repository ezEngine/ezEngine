#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

EZ_ALIGN_16(struct) PerInstanceData
{
	TRANSFORM(ObjectToWorld);
	TRANSFORM(ObjectToWorldNormal);
	INT1(GameObjectID);
	
	INT3(Reserved);
	FLOAT4(Reserved2);
};

#if EZ_ENABLED(PLATFORM_DX11)

	#if INSTANCING
		StructuredBuffer<PerInstanceData> perInstanceData;
	#else	
		CONSTANT_BUFFER(PerInstanceConstants, 2) 
		{
			PerInstanceData perInstanceData;
		};
	#endif
	
#endif
