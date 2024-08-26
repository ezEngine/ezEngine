#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER(ezGizmoConstants, 2)
{
  MAT4(ObjectToWorldMatrix);
  MAT4(WorldToObjectMatrix);
  COLOR4F(GizmoColor);
  FLOAT1(GizmoScale);
  INT1(GameObjectID);
};
