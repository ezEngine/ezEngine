#pragma once

#include <PhysXPlugin/Components/PhysXComponent.h>
#include <Core/Messages/TriggerMessage.h>

typedef ezComponentManagerSimple<class ezPxCharacterControllerComponent, true> ezPxCharacterControllerComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxCharacterControllerComponent : public ezPhysXComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxCharacterControllerComponent, ezPhysXComponent, ezPxCharacterControllerComponentManager);

public:
  ezPxCharacterControllerComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void Update();

  virtual void OnSimulationStarted() override;

  // ************************************* PROPERTIES ***********************************
public:

  float m_fWalkSpeed; ///< How many meters the character walks per second
  float m_fRunSpeed;
  float m_fAirSpeed; ///< How fast the character can change direction while not standing on solid round
  float m_fAirFriction; ///< How much damping is applied to the velocity when the character is jumping
  ezAngle m_RotateSpeed; ///< How many degrees per second the character turns
  float m_fJumpImpulse;

protected:


  // ************************************* FUNCTIONS *****************************

public:

  void TriggerMessageHandler(ezTriggerMessage& msg);

protected:
  ezComponentHandle m_hProxy;

  enum InputStateBits : ezUInt8
  {
    Jump = EZ_BIT(0),
    Crouch = EZ_BIT(1),
    Run = EZ_BIT(2),
  };

  ezUInt8 m_InputStateBits;
  ezVec3 m_vRelativeMoveDirection;
  ezAngle m_RotateZ;

  float m_fVelocityUp;
  ezVec3 m_vVelocityLateral;
};


