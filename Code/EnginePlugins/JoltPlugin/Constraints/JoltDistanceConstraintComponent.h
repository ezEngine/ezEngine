#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using ezJoltDistanceConstraintComponentManager = ezComponentManager<class ezJoltDistanceConstraintComponent, ezBlockStorageType::Compact>;

class EZ_JOLTPLUGIN_DLL ezJoltDistanceConstraintComponent : public ezJoltConstraintComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltDistanceConstraintComponent, ezJoltConstraintComponent, ezJoltDistanceConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltDistanceConstraintComponent

public:
  ezJoltDistanceConstraintComponent();
  ~ezJoltDistanceConstraintComponent();

  float GetMinDistance() const { return m_fMinDistance; } // [ property ]
  void SetMinDistance(float value);                       // [ property ]

  float GetMaxDistance() const { return m_fMaxDistance; } // [ property ]
  void SetMaxDistance(float value);                       // [ property ]

  void SetFrequency(float value);                     // [ property ]
  float GetFrequency() const { return m_fFrequency; } // [ property ]

  void SetDamping(float value);                   // [ property ]
  float GetDamping() const { return m_fDamping; } // [ property ]

  virtual void ApplySettings() final override;

protected:
  float m_fMinDistance = 0.0f;
  float m_fMaxDistance = 1.0f;
  float m_fFrequency = 0.0f;
  float m_fDamping = 0.0f;
};
