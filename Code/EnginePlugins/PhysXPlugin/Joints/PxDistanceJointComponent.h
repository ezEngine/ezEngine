#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

using ezPxDistanceJointComponentManager = ezComponentManager<class ezPxDistanceJointComponent, ezBlockStorageType::Compact>;

class EZ_PHYSXPLUGIN_DLL ezPxDistanceJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPxDistanceJointComponent, ezPxJointComponent, ezPxDistanceJointComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxJointComponent

protected:
  virtual void CreateJointType(
    physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxDistanceJointComponent

public:
  ezPxDistanceJointComponent();
  ~ezPxDistanceJointComponent();

  float GetMinDistance() const { return m_fMinDistance; }         // [ property ]
  void SetMinDistance(float value);                               // [ property ]

  float GetMaxDistance() const { return m_fMaxDistance; }         // [ property ]
  void SetMaxDistance(float value);                               // [ property ]

  void SetSpringStiffness(float value);                           // [ property ]
  float GetSpringStiffness() const { return m_fSpringStiffness; } // [ property ]

  void SetSpringDamping(float value);                             // [ property ]
  float GetSpringDamping() const { return m_fSpringDamping; }     // [ property ]

  void SetSpringTolerance(float value);                           // [ property ]
  float GetSpringTolerance() const { return m_fSpringTolerance; } // [ property ]

  virtual void ApplySettings() final override;

protected:
  float m_fMinDistance = 0.0f;
  float m_fMaxDistance = 1.0f;
  float m_fSpringStiffness = 0.0f;
  float m_fSpringDamping = 1.0f;
  float m_fSpringTolerance = 0.05f;
};
