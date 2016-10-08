#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezParticleSystemConstants, 2)
{
	MAT4(ObjectToWorldMatrix);
};
