#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

typedef ezComponentManager<class ezPxSphericalJointComponent, ezBlockStorageType::Compact> ezPxSphericalJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxSphericalJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxSphericalJointComponent, ezPxJointComponent, ezPxSphericalJointComponentManager);

public:
  ezPxSphericalJointComponent();
  ~ezPxSphericalJointComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  bool GetLimitRotation() const { return m_bLimitRotation; }
  void SetLimitRotation(bool b);

  ezAngle GetConeLimitY() const { return m_ConeLimitY; }
  void SetConeLimitY(ezAngle v);

  ezAngle GetConeLimitZ() const { return m_ConeLimitZ; }
  void SetConeLimitZ(ezAngle v);

protected:
  bool m_bLimitRotation = false;
  ezAngle m_ConeLimitY;
  ezAngle m_ConeLimitZ;


  void ApplyConeLimit(PxJoint* pJoint0);

  virtual physx::PxJoint* CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;
};


