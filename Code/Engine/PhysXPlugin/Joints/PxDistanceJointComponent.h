#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

typedef ezComponentManager<class ezPxDistanceJointComponent> ezPxDistanceJointComponentManager;

class EZ_PHYSXPLUGIN_DLL ezPxDistanceJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxDistanceJointComponent, ezPxJointComponent, ezPxDistanceJointComponentManager);
  virtual void ezPhysXComponentIsAbstract() override {}

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

  virtual ezComponent::Initialization Initialize() override { return ezComponent::Initialization::RequiresInit2; }
  virtual void Initialize2() override;

  virtual PxJoint* CreateJointType(PxPhysics& api, PxRigidActor* actor0, const PxTransform& localFrame0, PxRigidActor* actor1, const PxTransform& localFrame1) override;
};


