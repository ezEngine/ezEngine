#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

/// Base class for particle finalizers.
///
/// Finalizers run at the end of each simulation frame to update particle state or
/// perform cleanup operations. They execute after all behaviors have been processed.
/// Examples include updating particle age, applying velocity to position, or computing
/// bounding volumes.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer : public ezParticleModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizer, ezParticleModule);

public:
  ezParticleFinalizer();
};
