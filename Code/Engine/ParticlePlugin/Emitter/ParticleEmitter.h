#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementSpawner.h>

class ezParticleSystemInstance;
class ezStream;
class ezParticleEmitter;

/// \brief Base class for all particle emitters
class EZ_PARTICLEPLUGIN_DLL ezParticleEmitterFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitterFactory, ezReflectedClass);

public:
  ezParticleEmitterFactory();

  virtual const ezRTTI* GetEmitterType() const = 0;
  virtual void CopyEmitterProperties(ezParticleEmitter* pEmitter) const = 0;

  ezParticleEmitter* CreateEmitter(ezParticleSystemInstance* pOwner) const;

  virtual void Save(ezStreamWriter& stream) const = 0;
  virtual void Load(ezStreamReader& stream) = 0;
};

/// \brief Base class for stream spawners that are used by ezParticleEmitter's
class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter : public ezStreamElementSpawner
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter, ezStreamElementSpawner);

  friend class ezParticleSystemInstance;
  friend class ezParticleEmitterFactory;

  /// \brief Called after construction when m_pOwnerSystem is set and when properties have been set
  ///
  /// If bFirstTime is true, the emitter was just freshly created.
  /// If it is false, it was in use before and only the properties were changed, the emitter should
  /// continue operating as before.
  virtual void AfterPropertiesConfigured(bool bFirstTime) {}

protected:

  virtual ezResult UpdateStreamBindings() override;

  /// \brief Called once per update. Must return how many new particles are to be spawned.
  virtual ezUInt32 ComputeSpawnCount() = 0;

  ezParticleSystemInstance* m_pOwnerSystem;
  ezStream* m_pStreamPosition;
  ezStream* m_pStreamVelocity;
  ezStream* m_pStreamColor;
  ezStream* m_pStreamLifeTime;
};
