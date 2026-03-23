#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// What happens when a particle is outside the sphere
struct EZ_PARTICLEPLUGIN_DLL ezParticleSphereOutOfBoundsMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Kill,      ///< Remove particle immediately
    Constrain, ///< Clamp particle to sphere surface

    Default = Kill
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleSphereOutOfBoundsMode);

/// Constrains particles to a spherical volume.
///
/// Particles outside the radius are either killed or pushed back to the sphere surface.
/// The center is relative to the effect's world position.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_BoundsSphere final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_BoundsSphere, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_BoundsSphere();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

  ezVec3 m_vCenterOffset = ezVec3::MakeZero();
  float m_fRadius = 3.0f;
  ezEnum<ezParticleSphereOutOfBoundsMode> m_OutOfBoundsMode;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_BoundsSphere final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_BoundsSphere, ezParticleBehavior);

public:
  ezVec3 m_vCenterOffset = ezVec3::MakeZero();
  float m_fRadius = 3.0f;
  ezEnum<ezParticleSphereOutOfBoundsMode> m_OutOfBoundsMode;

protected:
  virtual void CreateRequiredStreams() override;
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition = nullptr;
};
