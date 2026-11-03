#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

/// Current state of a particle emitter
enum class ezParticleEmitterState
{
  Active,       ///< Emitter is actively spawning particles
  Finished,     ///< Emitter will not spawn more particles
  OnlyReacting, ///< Only spawns on events, considered finished when other emitters finish
};

/// Base class for all particle emitters
///
/// Emitters control when and how many particles are spawned.
class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter : public ezParticleModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter, ezParticleModule);

public:
  virtual void QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const = 0;

  virtual void Save(ezStreamWriter& inout_stream) const = 0;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) = 0;

  virtual bool IsContinuous() const { return false; }

  /// Called once per update. Must return how many new particles to spawn.
  virtual ezUInt32 ComputeSpawnCount(const ezParticleModuleContext& ctx, const ezTime& tDiff) const = 0;

  /// Called before ComputeSpawnCount(). Returns whether the emitter will spawn more particles.
  virtual ezParticleEmitterState IsFinished(const ezParticleModuleContext& ctx) const = 0;

  virtual void ProcessEventQueue(const ezParticleModuleContext& ctx, ezParticleEventQueue queue) const {}
};
