#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgMoveCharacterController);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgMoveCharacterController, 1, ezRTTIDefaultAllocator<ezMsgMoveCharacterController>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MoveForwards", m_fMoveForwards),
    EZ_MEMBER_PROPERTY("MoveBackwards", m_fMoveBackwards),
    EZ_MEMBER_PROPERTY("StrafeLeft", m_fStrafeLeft),
    EZ_MEMBER_PROPERTY("StrafeRight", m_fStrafeRight),
    EZ_MEMBER_PROPERTY("RotateLeft", m_fRotateLeft),
    EZ_MEMBER_PROPERTY("RotateRight", m_fRotateRight),
    EZ_MEMBER_PROPERTY("Run", m_bRun),
    EZ_MEMBER_PROPERTY("Jump", m_bJump),
    EZ_MEMBER_PROPERTY("Crouch", m_bCrouch),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_STATICLINK_FILE(GameEngine, GameEngine_Physics_Implementation_CharacterControllerComponent);
