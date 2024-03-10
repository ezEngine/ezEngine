#pragma once

#include <PhysXPlugin/Joints/PxJointComponent.h>

struct EZ_PHYSXPLUGIN_DLL ezPxAxis
{
  using StorageType = ezUInt8;

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
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPxJointComponent

protected:
  virtual void CreateJointType(
    physx::PxRigidActor* actor0, const physx::PxTransform& localFrame0, physx::PxRigidActor* actor1, const physx::PxTransform& localFrame1) override;


  //////////////////////////////////////////////////////////////////////////
  // ezPx6DOFJointComponent

public:
  ezPx6DOFJointComponent();
  ~ezPx6DOFJointComponent();

  virtual void ApplySettings() final override;

  void SetFreeLinearAxis(ezBitflags<ezPxAxis> flags);                               // [ property ]
  ezBitflags<ezPxAxis> GetFreeLinearAxis() const { return m_FreeLinearAxis; }       // [ property ]

  void SetFreeAngularAxis(ezBitflags<ezPxAxis> flags);                              // [ property ]
  ezBitflags<ezPxAxis> GetFreeAngularAxis() const { return m_FreeAngularAxis; }     // [ property ]

  void SetLinearLimitMode(ezPxJointLimitMode::Enum mode);                           // [ property ]
  ezPxJointLimitMode::Enum GetLinearLimitMode() const { return m_LinearLimitMode; } // [ property ]

  void SetLinearRangeX(const ezVec2& value);                                        // [ property ]
  const ezVec2& GetLinearRangeX() const { return m_vLinearRangeX; }                 // [ property ]
  void SetLinearRangeY(const ezVec2& value);                                        // [ property ]
  const ezVec2& GetLinearRangeY() const { return m_vLinearRangeY; }                 // [ property ]
  void SetLinearRangeZ(const ezVec2& value);                                        // [ property ]
  const ezVec2& GetLinearRangeZ() const { return m_vLinearRangeZ; }                 // [ property ]

  void SetLinearStiffness(float f);                                                 // [ property ]
  float GetLinearStiffness() const { return m_fLinearStiffness; }                   // [ property ]

  void SetLinearDamping(float f);                                                   // [ property ]
  float GetLinearDamping() const { return m_fLinearDamping; }                       // [ property ]

  void SetSwingLimitMode(ezPxJointLimitMode::Enum mode);                            // [ property ]
  ezPxJointLimitMode::Enum GetSwingLimitMode() const { return m_SwingLimitMode; }   // [ property ]

  void SetSwingLimit(ezAngle f);                                                    // [ property ]
  ezAngle GetSwingLimit() const { return m_SwingLimit; }                            // [ property ]

  void SetSwingStiffness(float f);                                                  // [ property ]
  float GetSwingStiffness() const { return m_fSwingStiffness; }                     // [ property ]

  void SetSwingDamping(float f);                                                    // [ property ]
  float GetSwingDamping() const { return m_fSwingDamping; }                         // [ property ]

  void SetTwistLimitMode(ezPxJointLimitMode::Enum mode);                            // [ property ]
  ezPxJointLimitMode::Enum GetTwistLimitMode() const { return m_TwistLimitMode; }   // [ property ]

  void SetLowerTwistLimit(ezAngle f);                                               // [ property ]
  ezAngle GetLowerTwistLimit() const { return m_LowerTwistLimit; }                  // [ property ]

  void SetUpperTwistLimit(ezAngle f);                                               // [ property ]
  ezAngle GetUpperTwistLimit() const { return m_UpperTwistLimit; }                  // [ property ]

  void SetTwistStiffness(float f);                                                  // [ property ]
  float GetTwistStiffness() const { return m_fTwistStiffness; }                     // [ property ]

  void SetTwistDamping(float f);                                                    // [ property ]
  float GetTwistDamping() const { return m_fTwistDamping; }                         // [ property ]

protected:
  ezBitflags<ezPxAxis> m_FreeLinearAxis;

  ezEnum<ezPxJointLimitMode> m_LinearLimitMode;

  float m_fLinearStiffness = 0.0f;
  float m_fLinearDamping = 0.0f;

  ezVec2 m_vLinearRangeX = ezVec2::MakeZero();
  ezVec2 m_vLinearRangeY = ezVec2::MakeZero();
  ezVec2 m_vLinearRangeZ = ezVec2::MakeZero();

  ezBitflags<ezPxAxis> m_FreeAngularAxis;

  ezEnum<ezPxJointLimitMode> m_SwingLimitMode;
  ezAngle m_SwingLimit;

  float m_fSwingStiffness = 0.0f; // [ property ]
  float m_fSwingDamping = 0.0f;   // [ property ]

  ezEnum<ezPxJointLimitMode> m_TwistLimitMode;
  ezAngle m_LowerTwistLimit;
  ezAngle m_UpperTwistLimit;

  float m_fTwistStiffness = 0.0f; // [ property ]
  float m_fTwistDamping = 0.0f;   // [ property ]
};
