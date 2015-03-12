#include <PCH.h>
#include <RTS/Rendering/Renderer.h>
#include <RTS/General/Application.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Core/Input/InputManager.h>

bool GameRenderer::GetPickingRay(float fMousePosX, float fMousePosY, ezVec3& out_RayPos, ezVec3& out_RayDir)
{
  ezVec3 vScreenPos;
  vScreenPos.x = fMousePosX;
  vScreenPos.y = 1.0f - fMousePosY;
  vScreenPos.z = 0.0f;

  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseModelViewProjectionMatrix, 0, 0, 1, 1, vScreenPos, out_RayPos, &out_RayDir) == EZ_SUCCESS)
    return true;

  return false;
}

ezVec2I32 SampleGameApp::GetPickedGridCell(ezVec3* out_vIntersection) const
{
  float fMouseX, fMouseY;
  ezInputManager::GetInputSlotState(ezInputSlot_MousePositionX, &fMouseX);
  ezInputManager::GetInputSlotState(ezInputSlot_MousePositionY, &fMouseY);

  ezVec3 vWorldPos;
  ezVec3 vDirToWorldPos;

  if (!m_pRenderer->GetPickingRay(fMouseX, fMouseY, vWorldPos, vDirToWorldPos))
    return ezVec2I32(-1);

  ezVec2I32 Coord;
  if (!m_pLevel->GetGrid().PickCell(vWorldPos, vDirToWorldPos, &Coord, out_vIntersection))
    return ezVec2I32(-1);

  return Coord;
}

