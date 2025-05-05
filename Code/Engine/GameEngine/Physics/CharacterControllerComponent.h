#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezMsgMoveCharacterController : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgMoveCharacterController, ezMessage);

  double m_fMoveForwards = 0;
  double m_fMoveBackwards = 0;
  double m_fStrafeLeft = 0;
  double m_fStrafeRight = 0;
  double m_fRotateLeft = 0;
  double m_fRotateRight = 0;
  bool m_bRun = false;
  bool m_bJump = false;
  bool m_bCrouch = false;
};
