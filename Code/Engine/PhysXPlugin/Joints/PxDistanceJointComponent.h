#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

struct ezCallDelayedStartMessage;

typedef ezComponentManager<class ezPxDistanceJointComponent, ezBlockStorageType::Compact> ezPxDistanceJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxDistanceJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxDistanceJointComponent, ezPxJointComponent, ezPxDistanceJointComponentManager);

public:
  ezPxDistanceJointComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  // ************************************* PROPERTIES ***********************************
public:

  float m_fMinDistance;
  float m_fMaxDistance;
  float m_fSpringStiffness;
  float m_fSpringDamping;
  float m_fSpringTolerance;


protected:


  // ************************************* FUNCTIONS *****************************

public:

protected:

  virtual void OnSimulationStarted() override;

  virtual physx::PxJoint* CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;
};


