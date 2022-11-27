#if 0

#  pragma once

#  include <JoltPlugin/Constraints/JoltConstraintComponent.h>

struct EZ_JOLTPLUGIN_DLL ezJoltAxis
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

EZ_DECLARE_FLAGS_OPERATORS(ezJoltAxis);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltAxis);

using ezJolt6DOFConstraintComponentManager = ezComponentManager<class ezJolt6DOFConstraintComponent, ezBlockStorageType::Compact>;

class EZ_JOLTPLUGIN_DLL ezJolt6DOFConstraintComponent : public ezJoltConstraintComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJolt6DOFConstraintComponent, ezJoltConstraintComponent, ezJolt6DOFConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;


  //////////////////////////////////////////////////////////////////////////
  // ezJolt6DOFConstraintComponent

public:
  ezJolt6DOFConstraintComponent();
  ~ezJolt6DOFConstraintComponent();

  virtual void ApplySettings() final override;

  void SetFreeLinearAxis(ezBitflags<ezJoltAxis> flags);                         // [ property ]
  ezBitflags<ezJoltAxis> GetFreeLinearAxis() const { return m_FreeLinearAxis; } // [ property ]

  void SetFreeAngularAxis(ezBitflags<ezJoltAxis> flags);                          // [ property ]
  ezBitflags<ezJoltAxis> GetFreeAngularAxis() const { return m_FreeAngularAxis; } // [ property ]

  void SetLinearLimitMode(ezJoltConstraintLimitMode::Enum mode);                           // [ property ]
  ezJoltConstraintLimitMode::Enum GetLinearLimitMode() const { return m_LinearLimitMode; } // [ property ]

  void SetLinearRangeX(const ezVec2& value);                        // [ property ]
  const ezVec2& GetLinearRangeX() const { return m_vLinearRangeX; } // [ property ]
  void SetLinearRangeY(const ezVec2& value);                        // [ property ]
  const ezVec2& GetLinearRangeY() const { return m_vLinearRangeY; } // [ property ]
  void SetLinearRangeZ(const ezVec2& value);                        // [ property ]
  const ezVec2& GetLinearRangeZ() const { return m_vLinearRangeZ; } // [ property ]

  void SetLinearStiffness(float f);                               // [ property ]
  float GetLinearStiffness() const { return m_fLinearStiffness; } // [ property ]

  void SetLinearDamping(float f);                             // [ property ]
  float GetLinearDamping() const { return m_fLinearDamping; } // [ property ]

  void SetSwingLimitMode(ezJoltConstraintLimitMode::Enum mode);                          // [ property ]
  ezJoltConstraintLimitMode::Enum GetSwingLimitMode() const { return m_SwingLimitMode; } // [ property ]

  void SetSwingLimit(ezAngle f);                         // [ property ]
  ezAngle GetSwingLimit() const { return m_SwingLimit; } // [ property ]

  void SetSwingStiffness(float f);                              // [ property ]
  float GetSwingStiffness() const { return m_fSwingStiffness; } // [ property ]

  void SetSwingDamping(float f);                            // [ property ]
  float GetSwingDamping() const { return m_fSwingDamping; } // [ property ]

  void SetTwistLimitMode(ezJoltConstraintLimitMode::Enum mode);                          // [ property ]
  ezJoltConstraintLimitMode::Enum GetTwistLimitMode() const { return m_TwistLimitMode; } // [ property ]

  void SetLowerTwistLimit(ezAngle f);                              // [ property ]
  ezAngle GetLowerTwistLimit() const { return m_LowerTwistLimit; } // [ property ]

  void SetUpperTwistLimit(ezAngle f);                              // [ property ]
  ezAngle GetUpperTwistLimit() const { return m_UpperTwistLimit; } // [ property ]

  void SetTwistStiffness(float f);                              // [ property ]
  float GetTwistStiffness() const { return m_fTwistStiffness; } // [ property ]

  void SetTwistDamping(float f);                            // [ property ]
  float GetTwistDamping() const { return m_fTwistDamping; } // [ property ]

protected:
  ezBitflags<ezJoltAxis> m_FreeLinearAxis;

  ezEnum<ezJoltConstraintLimitMode> m_LinearLimitMode;

  float m_fLinearStiffness = 0.0f;
  float m_fLinearDamping = 0.0f;

  ezVec2 m_vLinearRangeX = ezVec2::ZeroVector();
  ezVec2 m_vLinearRangeY = ezVec2::ZeroVector();
  ezVec2 m_vLinearRangeZ = ezVec2::ZeroVector();

  ezBitflags<ezJoltAxis> m_FreeAngularAxis;

  ezEnum<ezJoltConstraintLimitMode> m_SwingLimitMode;
  ezAngle m_SwingLimit;

  float m_fSwingStiffness = 0.0f; // [ property ]
  float m_fSwingDamping = 0.0f;   // [ property ]

  ezEnum<ezJoltConstraintLimitMode> m_TwistLimitMode;
  ezAngle m_LowerTwistLimit;
  ezAngle m_UpperTwistLimit;

  float m_fTwistStiffness = 0.0f; // [ property ]
  float m_fTwistDamping = 0.0f;   // [ property ]
};

#endif
