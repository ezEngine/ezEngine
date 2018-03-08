#pragma once

#include <PhysXPlugin/Components/PxComponent.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

struct ezPxCharacterCollisionFlags
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None = 0,
    Sides = EZ_BIT(0),
    Above = EZ_BIT(1),
    Below = EZ_BIT(2),

    Default = None
  };

  struct Bits
  {
    StorageType Sides : 1;
    StorageType Above : 1;
    StorageType Below : 1;
  };
};

struct ezPxCharacterProxyData;

typedef ezComponentManager<class ezPxCharacterProxyComponent, ezBlockStorageType::FreeList> ezPxCharacterProxyComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxCharacterProxyComponent : public ezPxComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxCharacterProxyComponent, ezPxComponent, ezPxCharacterProxyComponentManager);

public:
  ezPxCharacterProxyComponent();
  ~ezPxCharacterProxyComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  virtual void OnSimulationStarted() override;

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const;

  ezBitflags<ezPxCharacterCollisionFlags> Move(const ezVec3& vMotion, bool bCrouch);
  bool IsCrouching() const { return m_bIsCrouching; }
  float GetCurrentCapsuleHeight() const { return m_bIsCrouching ? m_fCapsuleCrouchHeight : m_fCapsuleHeight; }

  ezBitflags<ezPxCharacterCollisionFlags> GetCollisionFlags() const;
  bool IsGrounded() const { return GetCollisionFlags().IsSet(ezPxCharacterCollisionFlags::Below); }

  ezUInt32 GetShapeId() const { return m_uiShapeId; }

  // ************************************* PROPERTIES ***********************************
public:

  float m_fCapsuleHeight; ///< real character height is m_fCapsuleHeight + 2 * m_fCapsuleRadius
  float m_fCapsuleCrouchHeight; ///< real character height is m_fCapsuleHeight + 2 * m_fCapsuleRadius
  float m_fCapsuleRadius; ///< real character height is m_fCapsuleHeight + 2 * m_fCapsuleRadius
  float m_fMass; ///< mass is used to calculate pushing force from other rigid bodies
  float m_fMaxStepHeight; ///< how tall steps the character will climb automatically
  ezAngle m_MaxClimbingSlope; ///< Max slope angle that the character can climb before being stopped
  bool m_bForceSlopeSliding; ///< If standing on a steep slope, the character either can't walk up, or is even forced to slide down
  bool m_bConstrainedClimbingMode; ///< no idea what this does, try out or ask nVidia

  ezUInt8 m_uiCollisionLayer;

protected:
  ezUInt32 m_uiShapeId;
  bool m_bIsCrouching = false;

  physx::PxCapsuleController* m_pController;

  ezUniquePtr<ezPxCharacterProxyData> m_Data;

  ezPxUserData m_UserData;
};


