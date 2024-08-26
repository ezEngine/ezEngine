#pragma once

#include <PhysXPlugin/Components/PxComponent.h>

struct ezPxJointLimitMode
{
  using StorageType = ezInt8;

  enum Enum
  {
    NoLimit,
    HardLimit,
    SoftLimit,

    Default = NoLimit
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PHYSXPLUGIN_DLL, ezPxJointLimitMode);

struct ezPxJointDriveMode
{
  using StorageType = ezInt8;

  enum Enum
  {
    NoDrive,
    DriveAndSpin,
    DriveAndBrake,

    Default = NoDrive
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PHYSXPLUGIN_DLL, ezPxJointDriveMode);

//////////////////////////////////////////////////////////////////////////

class EZ_PHYSXPLUGIN_DLL ezPxJointComponent : public ezPxComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezPxJointComponent, ezPxComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxJointComponent

public:
  ezPxJointComponent();
  ~ezPxJointComponent();

  void SetBreakForce(float value);                            // [ property ]
  float GetBreakForce() const { return m_fBreakForce; }       // [ property ]

  void SetBreakTorque(float value);                           // [ property ]
  float GetBreakTorque() const { return m_fBreakTorque; }     // [ property ]

  void SetPairCollision(bool value);                          // [ property ]
  bool GetPairCollision() const { return m_bPairCollision; }  // [ property ]

  void SetParentActorReference(const char* szReference);      // [ property ]
  void SetChildActorReference(const char* szReference);       // [ property ]
  void SetChildActorAnchorReference(const char* szReference); // [ property ]

  void SetParentActor(ezGameObjectHandle hActor);
  void SetChildActor(ezGameObjectHandle hActor);
  void SetChildActorAnchor(ezGameObjectHandle hActor);

  void SetActors(ezGameObjectHandle hActorA, const ezTransform& localFrameA, ezGameObjectHandle hActorB, const ezTransform& localFrameB);

  virtual void ApplySettings() = 0;

  physx::PxJoint* GetPxJoint() { return m_pJoint; }

protected:
  ezResult FindParentBody(physx::PxRigidActor*& pActor);
  ezResult FindChildBody(physx::PxRigidActor*& pActor);

  virtual void CreateJointType(physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) = 0;

  void QueueApplySettings();

  ezGameObjectHandle m_hActorA;
  ezGameObjectHandle m_hActorB;
  ezGameObjectHandle m_hActorBAnchor;

  // UserFlag0 specifies whether m_localFrameA is already set
  ezTransform m_LocalFrameA;
  // UserFlag1 specifies whether m_localFrameB is already set
  ezTransform m_LocalFrameB;

  physx::PxJoint* m_pJoint = nullptr;

  float m_fBreakForce = 0.0f;
  float m_fBreakTorque = 0.0f;
  bool m_bPairCollision = false;

private:
  const char* DummyGetter() const { return nullptr; }
};
