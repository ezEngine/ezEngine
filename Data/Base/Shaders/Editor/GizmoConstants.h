#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(GizmoConstants, 2)
{
	MAT4(ObjectToWorldMatrix);
	COLOR(GizmoColor);
	FLOAT1(GizmoScale);
	INT1(GameObjectID);
};
