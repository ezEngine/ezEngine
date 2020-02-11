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

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PHYSXPLUGIN_DLL, ezPxCharacterCollisionFlags);

struct ezPxCharacterProxyData;

typedef ezComponentManager<class ezPxCharacterProxyComponent, ezBlockStorageType::FreeList> ezPxCharacterProxyComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxCharacterProxyComponent : public ezPxComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxCharacterProxyComponent, ezPxComponent, ezPxCharacterProxyComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxCharacterProxyComponent

public:
  ezPxCharacterProxyComponent();
  ~ezPxCharacterProxyComponent();

  ezBitflags<ezPxCharacterCollisionFlags> Move(const ezVec3& vMotion, bool bCrouch);                           // [ scriptable ]
  bool IsCrouching() const { return m_bIsCrouching; }                                                          // [ scriptable ]
  float GetCurrentCapsuleHeight() const { return m_bIsCrouching ? m_fCapsuleCrouchHeight : m_fCapsuleHeight; } // [ scriptable ]
  ezBitflags<ezPxCharacterCollisionFlags> GetCollisionFlags() const;                                           // [ scriptable ]
  bool IsGrounded() const { return GetCollisionFlags().IsSet(ezPxCharacterCollisionFlags::Below); }            // [ scriptable ]

  ezGameObjectHandle GetTouchedShapeObject() const; // TODO: make scriptable
  ezGameObjectHandle GetTouchedActorObject() const; // TODO: make scriptable

  ezUInt32 GetShapeId() const { return m_uiShapeId; } // [ scriptable ]

  float m_fCapsuleHeight = 1.0f;                       ///< [ property ] real character height is m_fCapsuleHeight + 2 * m_fCapsuleRadius
  float m_fCapsuleCrouchHeight = 0.2f;                 ///< [ property ] real character height is m_fCapsuleHeight + 2 * m_fCapsuleRadius
  float m_fCapsuleRadius = 0.25f;                      ///< [ property ] real character height is m_fCapsuleHeight + 2 * m_fCapsuleRadius
  float m_fMass = 100.0f;                              ///< [ property ] mass is used to calculate pushing force from other rigid bodies
  float m_fMaxStepHeight = 0.3f;                       ///< [ property ] how tall steps the character will climb automatically
  ezAngle m_MaxClimbingSlope = ezAngle::Degree(40.0f); ///< [ property ] Max slope angle that the character can climb before being stopped
  bool m_bForceSlopeSliding = true;                    ///< [ property ] If standing on a steep slope, the character either can't walk up, or is even forced to slide down
  bool m_bConstrainedClimbingMode = false;             ///< [ property ] no idea what this does, try out or ask nVidia

  ezUInt8 m_uiCollisionLayer = 0; // [ property ]

protected:
  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const; // [ msg handler ]

  ezUInt32 m_uiShapeId = ezInvalidIndex;
  bool m_bIsCrouching = false;

  physx::PxCapsuleController* m_pController = nullptr;

  ezUniquePtr<ezPxCharacterProxyData> m_Data;

  ezPxUserData m_UserData;
};
