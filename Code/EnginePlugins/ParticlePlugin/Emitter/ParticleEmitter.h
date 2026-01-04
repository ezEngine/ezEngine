#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezParticleSystemInstance;
class ezProcessingStream;
class ezParticleEmitter;

/// Base class for all particle emitters
class EZ_PARTICLEPLUGIN_DLL ezParticleEmitterFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitterFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetEmitterType() const = 0;
  virtual void CopyEmitterProperties(ezParticleEmitter* pEmitter, bool bFirstTime) const = 0;

  ezParticleEmitter* CreateEmitter(ezParticleSystemInstance* pOwner) const;
  virtual void QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const = 0;

  virtual void Save(ezStreamWriter& inout_stream) const = 0;
  virtual void Load(ezStreamReader& inout_stream) = 0;
};

/// Current state of a particle emitter
enum class ezParticleEmitterState
{
  Active,       ///< Emitter is actively spawning particles
  Finished,     ///< Emitter will not spawn more particles
  OnlyReacting, ///< Only spawns on events, considered finished when other emitters finish
};

/// Base class for particle emitters
///
/// Emitters control when and how many particles are spawned.
class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter : public ezParticleModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter, ezParticleModule);

  friend class ezParticleSystemInstance;
  friend class ezParticleEmitterFactory;

protected:
  virtual bool IsContinuous() const;
  virtual void Process(ezUInt64 uiNumElements) final override;

  /// Called once per update. Must return how many new particles to spawn.
  virtual ezUInt32 ComputeSpawnCount(const ezTime& tDiff) = 0;

  /// Called before ComputeSpawnCount(). Returns whether the emitter will spawn more particles.
  virtual ezParticleEmitterState IsFinished() = 0;

  virtual void ProcessEventQueue(ezParticleEventQueue queue);
};
