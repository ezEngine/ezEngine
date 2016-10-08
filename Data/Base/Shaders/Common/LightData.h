#pragma once

#include "Platforms.h"
#include "ConstantBufferMacros.h"

#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_SPOT 1
#define LIGHT_TYPE_DIR 2
#define LIGHT_TYPE_MASK 0x7

#define LIGHT_HAS_SHADOWS (1 << 3)
#define LIGHT_HAS_PROJECTOR (1 << 4)

EZ_ALIGN_16(struct) ezPerLightData
{
	UINT1(colorAndType);
	FLOAT1(intensity);
	UINT1(direction); // 10 bits fixed point per axis
	UINT1(shadowDataIndex);

	FLOAT3(position);
	FLOAT1(invSqrAttRadius);

	UINT1(spotParams); // scale and offset as 16 bit floats
	UINT1(projectorAtlasOffset); // xy as 16 bit floats
	UINT1(projectorAtlasScale); // xy as 16 bit floats

	UINT1(reserved);
};

#if EZ_ENABLED(PLATFORM_DX11)
	StructuredBuffer<ezPerLightData> perLightData;
#else
  EZ_CHECK_AT_COMPILETIME(sizeof(ezPerLightData) == 48);
#endif

CONSTANT_BUFFER(ezClusteredDataConstants, 3)
{
	UINT1(NumLights);
	UINT3(Padding);

	COLOR4F(AmbientTopColor);
	COLOR4F(AmbientBottomColor);
};
