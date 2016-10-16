#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

struct EZ_ALIGN_16(ezPerInstanceData)
{
	TRANSFORM(ObjectToWorld);
	TRANSFORM(ObjectToWorldNormal);
	INT1(GameObjectID);

	INT3(Reserved);
	FLOAT4(Reserved2);
};

#if EZ_ENABLED(PLATFORM_DX11)

	#if INSTANCING
		StructuredBuffer<ezPerInstanceData> perInstanceData;
	#else
		CONSTANT_BUFFER(ezPerInstanceConstants, 2)
		{
			ezPerInstanceData perInstanceData;
		};
	#endif

#else
  EZ_CHECK_AT_COMPILETIME(sizeof(ezPerInstanceData) == 128);
#endif
