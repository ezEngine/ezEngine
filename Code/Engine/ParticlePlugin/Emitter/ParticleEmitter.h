#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementSpawner.h>
#include <ParticlePlugin/Base/ParticleBase.h>

class ezParticleSystemInstance;
class ezStream;
class ezParticleEmitter;

/// \brief Base class for all particle emitters
class EZ_PARTICLEPLUGIN_DLL ezParticleEmitterFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitterFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetEmitterType() const = 0;
  virtual void CopyEmitterProperties(ezParticleEmitter* pEmitter) const = 0;

  ezParticleEmitter* CreateEmitter(ezParticleSystemInstance* pOwner) const;

  virtual void Save(ezStreamWriter& stream) const = 0;
  virtual void Load(ezStreamReader& stream) = 0;
};

/// \brief Base class for stream spawners that are used by ezParticleEmitter's
class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter : public ezParticleBase<ezStreamElementSpawner, true>
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter, ezStreamElementSpawner);

  friend class ezParticleSystemInstance;
  friend class ezParticleEmitterFactory;

protected:

  /// \brief Called once per update. Must return how many new particles are to be spawned.
  virtual ezUInt32 ComputeSpawnCount(const ezTime& tDiff) = 0;

  /// \brief Called before ComputeSpawnCount(). Should return true, if the emitter will never spawn any more particles.
  virtual bool IsFinished() = 0;

};
