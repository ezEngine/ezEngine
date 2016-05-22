#pragma once

#include <PhysXPlugin/Components/PhysXComponent.h>
#include <Core/Messages/TriggerMessage.h>

typedef ezComponentManagerSimple<class ezPxCharacterControllerComponent, true> ezPxCharacterControllerComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxCharacterControllerComponent : public ezPhysXComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxCharacterControllerComponent, ezPhysXComponent, ezPxCharacterControllerComponentManager);
  virtual void ezPhysXComponentIsAbstract() override {}

public:
  ezPxCharacterControllerComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void Update();

  virtual ezComponent::Initialization Initialize() override;

  virtual void Deinitialize() override;

  // ************************************* PROPERTIES ***********************************
public:

  float m_fCapsuleHeight; ///< real character height is m_fCapsuleHeight + 2 * m_fCapsuleRadius
  float m_fCapsuleRadius; ///< real character height is m_fCapsuleHeight + 2 * m_fCapsuleRadius
  float m_fMaxStepHeight; ///< how tall steps the character will climb automatically
  float m_fWalkSpeed; ///< How many meters the character walks per second
  float m_fRunSpeed;
  float m_fAirSpeed; ///< How fast the character can change direction while not standing on solid round
  float m_fAirFriction; ///< How much damping is applied to the velocity when the character is jumping
  ezAngle m_RotateSpeed; ///< How many degrees per second the character turns
  ezAngle m_MaxClimbingSlope; ///< Max slope angle that the character can climb before being stopped
  bool m_bForceSlopeSliding; ///< If standing on a steep slope, the character either can't walk up, or is even forced to slide down
  bool m_bConstrainedClimbingMode; ///< no idea what this does, try out or ask nVidia
  float m_fJumpImpulse;
  
  ezUInt8 m_uiCollisionLayer;

  //ezSurfaceResourceHandle m_hCharacterMaterial; // not sure this is needed for anything
  //float m_fInvisibleWallHeight; ??
  //float m_fMaxJumpHeight; ??


protected:


  // ************************************* FUNCTIONS *****************************

public:

  void TriggerMessageHandler(ezTriggerMessage& msg);

protected:
  enum InputStateBits : ezUInt8
  {
    Jump = EZ_BIT(0),
    Crouch = EZ_BIT(1),
    Run = EZ_BIT(2),
  };

  ezUInt8 m_InputStateBits;
  PxCapsuleController* m_pController;
  ezVec3 m_vRelativeMoveDirection;
  ezAngle m_RotateZ;


  float m_fVelocityUp;
  ezVec3 m_vVelocityLateral;
};


