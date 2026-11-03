#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

/// Base class for all particle behaviors
///
/// Behaviors update all active particles every simulation step. They run after the initializers and before the finalizers.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior : public ezParticleModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior, ezParticleModule);

public:
  virtual void Save(ezStreamWriter& inout_stream) const = 0;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) = 0;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const {}
};
