#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

struct EZ_PHYSXPLUGIN_DLL ezPxAxis
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None = 0,
    X = EZ_BIT(0),
    Y = EZ_BIT(1),
    Z = EZ_BIT(2),
    All = X | Y | Z,
    Default = All
  };

  struct Bits
  {
    StorageType X : 1;
    StorageType Y : 1;
    StorageType Z : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezPxAxis);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_PHYSXPLUGIN_DLL, ezPxAxis);

using ezPx6DOFJointComponentManager = ezComponentManager<class ezPx6DOFJointComponent, ezBlockStorageType::Compact>;

class EZ_PHYSXPLUGIN_DLL ezPx6DOFJointComponent : public ezPxJointComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPx6DOFJointComponent, ezPxJointComponent, ezPx6DOFJointComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxJointComponent

protected:
  virtual void CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPx6DOFJointComponent

public:
  ezPx6DOFJointComponent();
  ~ezPx6DOFJointComponent();

  ezBitflags<ezPxAxis> m_FreeLinearAxis;  // [ property ]
  ezBitflags<ezPxAxis> m_FreeAngularAxis; // [ property ]

  float m_fLinearStiffness = 0.0f;  // [ property ]
  float m_fLinearDamping = 0.0f;    // [ property ]
  float m_fAngularStiffness = 0.0f; // [ property ]
  float m_fAngularDamping = 0.0f;   // [ property ]

  virtual void ApplySettings() final override;

protected:
};
