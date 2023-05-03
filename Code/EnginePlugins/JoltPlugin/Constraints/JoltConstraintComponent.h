#pragma once

#include <Core/World/ComponentManager.h>
#include <JoltPlugin/Declarations.h>

class ezJoltDynamicActorComponent;

namespace JPH
{
  class Body;
}

namespace JPH
{
  class Constraint;
}

struct ezJoltConstraintLimitMode
{
  using StorageType = ezInt8;

  enum Enum
  {
    NoLimit,
    HardLimit,
    // SoftLimit,

    Default = NoLimit
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltConstraintLimitMode);

struct ezJoltConstraintDriveMode
{
  using StorageType = ezInt8;

  enum Enum
  {
    NoDrive,
    DriveVelocity,
    DrivePosition,

    Default = NoDrive
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltConstraintDriveMode);

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltConstraintComponent : public ezComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezJoltConstraintComponent, ezComponent);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltConstraintComponent

public:
  ezJoltConstraintComponent();
  ~ezJoltConstraintComponent();

  /// \brief Removes the connection between the joined bodies. This cannot be reversed.
  void BreakConstraint();

  void SetBreakForce(float value);                      // [ property ]
  float GetBreakForce() const { return m_fBreakForce; } // [ property ]

  void SetBreakTorque(float value);                       // [ property ]
  float GetBreakTorque() const { return m_fBreakTorque; } // [ property ]

  void SetPairCollision(bool value);                         // [ property ]
  bool GetPairCollision() const { return m_bPairCollision; } // [ property ]

  void SetParentActorReference(const char* szReference);      // [ property ]
  void SetChildActorReference(const char* szReference);       // [ property ]
  void SetChildActorAnchorReference(const char* szReference); // [ property ]

  void SetParentActor(ezGameObjectHandle hActor);
  void SetChildActor(ezGameObjectHandle hActor);
  void SetChildActorAnchor(ezGameObjectHandle hActor);

  void SetActors(ezGameObjectHandle hActorA, const ezTransform& localFrameA, ezGameObjectHandle hActorB, const ezTransform& localFrameB);

  virtual void ApplySettings() = 0;

  virtual bool ExceededBreakingPoint() = 0;

  /// \brief Forwards to BreakConstraint().
  void OnJoltMsgDisconnectConstraints(ezJoltMsgDisconnectConstraints& msg); // [ msg handler ]

protected:
  ezResult FindParentBody(ezUInt32& out_uiJoltBodyID, ezJoltDynamicActorComponent*& pRbComp);
  ezResult FindChildBody(ezUInt32& out_uiJoltBodyID, ezJoltDynamicActorComponent*& pRbComp);

  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) = 0;

  ezTransform ComputeParentBodyGlobalFrame() const;
  ezTransform ComputeChildBodyGlobalFrame() const;

  void QueueApplySettings();

  ezGameObjectHandle m_hActorA;
  ezGameObjectHandle m_hActorB;
  ezGameObjectHandle m_hActorBAnchor;

  // UserFlag0 specifies whether m_localFrameA is already set
  ezTransform m_LocalFrameA;
  // UserFlag1 specifies whether m_localFrameB is already set
  ezTransform m_LocalFrameB;

  JPH::Constraint* m_pConstraint = nullptr;

  float m_fBreakForce = 0.0f;
  float m_fBreakTorque = 0.0f;
  bool m_bPairCollision = true;

private:
  const char* DummyGetter() const { return nullptr; }
};
