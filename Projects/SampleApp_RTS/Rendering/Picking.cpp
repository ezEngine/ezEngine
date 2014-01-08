#include <SampleApp_RTS/Rendering/Renderer.h>
#include <SampleApp_RTS/General/Application.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Core/Input/InputManager.h>

bool GameRenderer::GetPickingRay(float fMousePosX, float fMousePosY, ezVec3& out_RayPos, ezVec3& out_RayDir)
{
  ezVec3 vScreenPos;
  vScreenPos.x = fMousePosX;
  vScreenPos.y = 1.0f - fMousePosY;
  vScreenPos.z = 0.0f;

  if (ezGraphicsUtils::ConvertScreenPosToWorldPos(m_InverseModelViewProjectionMatrix, ezProjectionDepthRange::MinusOneToOne, 0, 0, 1, 1, vScreenPos, out_RayPos, &out_RayDir) == EZ_SUCCESS)
    return true;

  return false;
}

ezGridCoordinate SampleGameApp::GetPickedGridCell() const
{
  float fMouseX, fMouseY;
  ezInputManager::GetInputSlotState(ezInputSlot_MousePositionX, &fMouseX);
  ezInputManager::GetInputSlotState(ezInputSlot_MousePositionY, &fMouseY);

  ezVec3 vWorldPos;
  ezVec3 vDirToWorldPos;

  if (!m_pRenderer->GetPickingRay(fMouseX, fMouseY, vWorldPos, vDirToWorldPos))
    return ezGridCoordinate(-1, -1, -1);

  return m_pLevel->GetGrid().PickCell(vWorldPos, vDirToWorldPos, NULL, 0);
}

