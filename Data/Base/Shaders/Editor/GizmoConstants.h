#pragma once

#include "../Common/Platforms.h"
#include "../Common/ConstantBufferMacros.h"

CONSTANT_BUFFER(ezGizmoConstants, 2)
{
	MAT4(ObjectToWorldMatrix);
	COLOR4F(GizmoColor);
	FLOAT1(GizmoScale);
	INT1(GameObjectID);
};
