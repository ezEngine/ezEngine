#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <Core/Messages/ScriptFunctionMessage.h>

//////////////////////////////////////////////////////////////////////////

struct EZ_GAMEENGINE_DLL ezCharacterController_MoveCharacterMsg : public ezScriptFunctionMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezCharacterController_MoveCharacterMsg, ezScriptFunctionMessage);

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

class EZ_GAMEENGINE_DLL ezCharacterControllerComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezCharacterControllerComponent, ezComponent);

public:
  ezCharacterControllerComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void MoveCharacter(ezCharacterController_MoveCharacterMsg& msg) = 0;
  virtual void RawMove(const ezVec3& vMove) = 0;
};


