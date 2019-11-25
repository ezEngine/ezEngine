#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

typedef ezComponentManager<class ezPxSphericalJointComponent, ezBlockStorageType::Compact> ezPxSphericalJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxSphericalJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxSphericalJointComponent, ezPxJointComponent, ezPxSphericalJointComponentManager);


  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxJointComponent

protected:
  virtual physx::PxJoint* CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxSphericalJointComponent

public:
  ezPxSphericalJointComponent();
  ~ezPxSphericalJointComponent();

  bool GetLimitRotation() const { return m_bLimitRotation; } // [ property ]
  void SetLimitRotation(bool b);                             // [ property ]

  ezAngle GetConeLimitY() const { return m_ConeLimitY; } // [ property ]
  void SetConeLimitY(ezAngle v);                         // [ property ]

  ezAngle GetConeLimitZ() const { return m_ConeLimitZ; } // [ property ]
  void SetConeLimitZ(ezAngle v);                         // [ property ]

protected:
  bool m_bLimitRotation = false;
  ezAngle m_ConeLimitY;
  ezAngle m_ConeLimitZ;

  void ApplyConeLimit(physx::PxJoint* pJoint0);
};
