#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

typedef ezComponentManager<class ezPxDistanceJointComponent, ezBlockStorageType::Compact> ezPxDistanceJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxDistanceJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxDistanceJointComponent, ezPxJointComponent, ezPxDistanceJointComponentManager);

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
  // ezPxDistanceJointComponent

public:
  ezPxDistanceJointComponent();
  ~ezPxDistanceJointComponent();

  float m_fMinDistance = 0.0f;     // [ property ]
  float m_fMaxDistance = 1.0f;     // [ property ]
  float m_fSpringStiffness = 0.0f; // [ property ]
  float m_fSpringDamping = 0.0f;   // [ property ]
  float m_fSpringTolerance = 0.0f; // [ property ]
};
