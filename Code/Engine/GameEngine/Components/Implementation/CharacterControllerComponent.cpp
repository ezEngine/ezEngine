#include <PCH.h>
#include <GameEngine/Components/CharacterControllerComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_MESSAGE_TYPE(ezCharacterController_MoveCharacterMsg);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCharacterController_MoveCharacterMsg, 1, ezRTTIDefaultAllocator<ezCharacterController_MoveCharacterMsg>)
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
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezCharacterControllerComponent, 1)
{
  //EZ_BEGIN_PROPERTIES
  //EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Game"),
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezCharacterController_MoveCharacterMsg, MoveCharacter),
  }
  EZ_END_MESSAGEHANDLERS
}
EZ_END_ABSTRACT_COMPONENT_TYPE

ezCharacterControllerComponent::ezCharacterControllerComponent()
{
}

void ezCharacterControllerComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

}

void ezCharacterControllerComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

}

