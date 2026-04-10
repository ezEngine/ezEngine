#pragma once

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/Component.h>
#include <Core/World/SpatialData.h>
#include <Core/World/World.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

using ezParticleAttractorComponentManager = ezComponentManager<class ezParticleAttractorComponent, ezBlockStorageType::Compact>;

/// \brief Defines an attractor (or repulsor) volume for nearby particles.
///
/// Particle systems with the "Attractors" behavior mode find these components via the spatial
/// system and apply their force per particle. All attractor components share the same spatial
/// category ("ParticleAttractor") so they can all be found with a single query.
///
/// Negative \a Strength values turn the attractor into a repulsor.
class EZ_PARTICLEPLUGIN_DLL ezParticleAttractorComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezParticleAttractorComponent, ezComponent, ezParticleAttractorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezParticleAttractorComponent

public:
  ezParticleAttractorComponent();
  ~ezParticleAttractorComponent();

  /// \brief Influence radius. Particles further away than this are not affected.
  float m_fRadius = 5.0f; // [ property ]

  /// \brief Attraction force applied per second at full strength. Negative values repel.
  float m_fStrength = 5.0f; // [ property ]

  /// \brief Minimum distance from the attractor center. Prevents singularity effects at the center.
  float m_fMinDistance = 0.1f; // [ property ]

  /// \brief Particles that come closer than this distance are destroyed. Zero disables the kill zone.
  float m_fKillDistance = 0.0f; // [ property ]

  /// \brief The spatial category shared by all particle attractor components.
  static ezSpatialData::Category GetSpatialCategory();

protected:
  void OnMsgUpdateLocalBounds(ezMsgUpdateLocalBounds& msg) const; // [ msg handler ]
};
