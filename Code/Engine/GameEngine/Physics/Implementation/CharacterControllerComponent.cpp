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
  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezCharacterControllerComponent, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay"),
    new ezColorAttribute(ezColorScheme::GetGroupColor(ezColorScheme::Gameplay)),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(RawMove, In, "moveDeltaGlobal"),
    EZ_SCRIPT_FUNCTION_PROPERTY(TeleportCharacter, In, "globalFootPosition"),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsDestinationUnobstructed, In, "globalFootPosition", In, "characterHeight"),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsTouchingGround),
    EZ_SCRIPT_FUNCTION_PROPERTY(IsCrouching),
  }
  EZ_END_FUNCTIONS;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgMoveCharacterController, MoveCharacter),
  }
  EZ_END_MESSAGEHANDLERS;
}
EZ_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

ezCharacterControllerComponent::ezCharacterControllerComponent() = default;

void ezCharacterControllerComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  // auto& s = stream.GetStream();
}

void ezCharacterControllerComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  // auto& s = stream.GetStream();
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Physics_Implementation_CharacterControllerComponent);
