#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezParticleSystemInstance;
class ezProcessingStream;
class ezParticleInitializer;
class ezParticleEffectInstance;

/// Base class for all particle initializers
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializerFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializerFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetInitializerType() const = 0;
  virtual void CopyInitializerProperties(ezParticleInitializer* pInitializer, bool bFirstTime) const = 0;
  virtual float GetSpawnCountMultiplier(const ezParticleEffectInstance* pEffect) const;

  ezParticleInitializer* CreateInitializer(ezParticleSystemInstance* pOwner) const;

  virtual void Save(ezStreamWriter& inout_stream) const = 0;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) = 0;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const {}
};

/// Base class for particle initializers
///
/// Initializers set the initial values of newly spawned particles.
/// They are executed once per particle when it is created.
class EZ_PARTICLEPLUGIN_DLL ezParticleInitializer : public ezParticleModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleInitializer, ezParticleModule);

  friend class ezParticleSystemInstance;
  friend class ezParticleInitializerFactory;

public:
  /// Whether the initializer has everything it needs to initialize new particles.
  ///
  /// Called before pre-simulating and before any particles are emitted, possibly several times per frame and from a task.
  /// As long as any initializer returns false, the system's emitters are not updated, so spawning is postponed (including bursts and start delays).
  /// Used by initializers that need data from the world, which they only get in ezParticleModule::UpdateFromWorld(),
  /// or resources which may still be streaming in. Not const, so that an initializer can pick up such data here.
  virtual bool PrepareToSpawn() { return true; }

  /// Scales the spawn counts of the emitters (and the max particle count) of the owning system, for values that are only known per instance.
  ///
  /// In contrast to ezParticleInitializerFactory::GetSpawnCountMultiplier() this may depend on data that only becomes available at runtime,
  /// e.g. the size of a spawn area or the surface area of the owner mesh. Only queried once PrepareToSpawn() returned true.
  /// If the value differs from the one the system currently uses, the effect gets reconfigured in the next frame, nothing is spawned until then.
  /// If that changes the max particle count, the system is recreated, which resets it. So this should only change rarely, ideally once.
  /// The value is reduced, with a warning, if it would raise the max particle count of the system above 20000.
  virtual float GetSpawnCountMultiplier() const { return 1.0f; }

protected:
  ezParticleInitializer();

  virtual void Process(ezUInt64 uiNumElements) final override {}
};
