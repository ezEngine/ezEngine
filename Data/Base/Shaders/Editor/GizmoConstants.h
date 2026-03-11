#pragma once

#include "../Common/ConstantBufferMacros.h"
#include "../Common/Platforms.h"

CONSTANT_BUFFER2(ezGizmoConstants, 2, BG_RENDER_PASS)
{
  MAT4(ObjectToWorldMatrix);
  MAT4(WorldToObjectMatrix);
  COLOR4F(GizmoColor);
  FLOAT1(GizmoScale);
  INT1(GameObjectID);
};
