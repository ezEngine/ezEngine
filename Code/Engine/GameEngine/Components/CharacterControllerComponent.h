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

/// \brief Base class for implementations of a character controller.
///
/// A character controller implements the raw physical locomotion aspect of moving a character (player or NPC)
/// through the world. This is typically implemented through a physics engine, when natural collision behavior
/// is desired. In more constricted settings, it could use a much simpler implementation.
class EZ_GAMEENGINE_DLL ezCharacterControllerComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezCharacterControllerComponent, ezComponent);

public:
  ezCharacterControllerComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  /// \brief Instructs the CC to move in certain directions. An implementation can queue the request for later processing.
  /// It can also add further functionality, such as adding gravity, stair stepping, etc.
  virtual void MoveCharacter(ezCharacterController_MoveCharacterMsg& msg) = 0;

  /// \brief Attempts to move the character into the given direction.
  /// 
  /// Implements the specific constraints, such as colliding with geometry and sliding along walls.
  /// Should NOT add further functionality, such as gravity. This function applies the result immediately
  /// and moves the owner object to the final location.
  virtual void RawMove(const ezVec3& vMove) = 0;
};


