#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using ezJoltDistanceConstraintComponentManager = ezComponentManager<class ezJoltDistanceConstraintComponent, ezBlockStorageType::Compact>;

/// \brief Implements a distance physics constraint.
///
/// The two joined actors have to keep a minimum and maximum distance between each other.
/// They are pushed apart if they come too close, and pulled together if they separate too much.
///
/// To push and pull may be very abrupt, like pieces of a chain, or elastic, like in a rubber band.
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
  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltDistanceConstraintComponent

public:
  ezJoltDistanceConstraintComponent();
  ~ezJoltDistanceConstraintComponent();

  float GetMinDistance() const { return m_fMinDistance; } // [ property ]
  void SetMinDistance(float value);                       // [ property ]

  float GetMaxDistance() const { return m_fMaxDistance; } // [ property ]
  void SetMaxDistance(float value);                       // [ property ]

  /// \brief Determines how often (per second) the constraint is enforced.
  ///
  /// Higher values make the constraint stiffer, but can also lead to oscillation. Good values are in range 0.1 to 20.
  void SetFrequency(float value);                     // [ property ]
  float GetFrequency() const { return m_fFrequency; } // [ property ]

  /// \brief How much to dampen actors when they overshoot the target position.
  ///
  /// Lower values make the objects bounce back harder, higher values make them just stop.
  void SetDamping(float value);                   // [ property ]
  float GetDamping() const { return m_fDamping; } // [ property ]

protected:
  float m_fMinDistance = 0.0f;
  float m_fMaxDistance = 1.0f;
  float m_fFrequency = 0.0f;
  float m_fDamping = 0.0f;
};
