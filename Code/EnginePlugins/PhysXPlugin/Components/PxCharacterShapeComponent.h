#pragma once

#include <PhysXPlugin/Components/PxComponent.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

struct ezPhysicsCastResult;

struct ezPxCharacterShapeCollisionFlags
{
  using StorageType = ezUInt32;

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

  static ezBitflags<ezPxCharacterShapeCollisionFlags> FromPxFlags(ezUInt32 uiFlags);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PHYSXPLUGIN_DLL, ezPxCharacterShapeCollisionFlags);

class EZ_PHYSXPLUGIN_DLL ezPxCharacterShapeComponent : public ezPxComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezPxCharacterShapeComponent, ezPxComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxCharacterShapeComponent

public:
  ezPxCharacterShapeComponent();
  ~ezPxCharacterShapeComponent();

  float m_fMass = 100.0f;                                                                                      ///< [ property ] mass is used to calculate pushing force from other rigid bodies
  float m_fMaxStepHeight = 0.3f;                                                                               ///< [ property ] how tall steps the character will climb automatically
  ezAngle m_MaxClimbingSlope = ezAngle::MakeFromDegree(40.0f);                                                 ///< [ property ] Max slope angle that the character can climb before being stopped
  bool m_bForceSlopeSliding = true;                                                                            ///< [ property ] If standing on a steep slope, the character either can't walk up, or is even forced to slide down
  bool m_bConstrainedClimbingMode = false;                                                                     ///< [ property ] no idea what this does, try out or ask NVIDIA
  ezUInt8 m_uiCollisionLayer = 0;                                                                              ///< [ property ] What other geometry the CC will collide with

  ezUInt32 GetShapeId() const { return m_uiShapeId; }                                                          // [ scriptable ] The ID of the CC's shape/actor
  ezBitflags<ezPxCharacterShapeCollisionFlags> GetCollisionFlags() const;                                      // [ scriptable ]

  bool IsTouchingGround() const { return GetCollisionFlags().IsSet(ezPxCharacterShapeCollisionFlags::Below); } // [ scriptable ]

  ezGameObjectHandle GetStandingOnShape() const;                                                               // TODO: make scriptable
  ezGameObjectHandle GetStandingOnActor() const;                                                               // TODO: make scriptable

  virtual ezBitflags<ezPxCharacterShapeCollisionFlags> MoveShape(const ezVec3& vMoveDelta) = 0;
  virtual void TeleportShape(const ezVec3& vGlobalFootPos);

  float GetCurrentHeightValue() const { return m_fCurrentHeightValue; } // [ scriptable ]
  virtual float GetCurrentTotalHeight() = 0;

  virtual bool TestShapeSweep(ezPhysicsCastResult& out_sweepResult, const ezVec3& vDirGlobal, float fDistance) = 0;
  virtual bool TestShapeOverlap(const ezVec3& vGlobalFootPos, float fNewHeightValue) = 0;
  virtual bool CanResize(float fNewHeightValue);
  virtual bool TryResize(float fNewHeightValue);

protected:
  ezPxUserData* GetUserData();

  float m_fCurrentHeightValue = 0.0f;
  physx::PxController* m_pController = nullptr;

private:
  ezUInt32 m_uiUserDataIndex = ezInvalidIndex;
  ezUInt32 m_uiShapeId = ezInvalidIndex;
};
