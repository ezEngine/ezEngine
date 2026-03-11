#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezProcessingStream;
class ezParticleSystemInstance;
class ezParticleFinalizer;

/// Factory class for creating particle finalizer instances.
///
/// Finalizer factories hold the configuration properties and create the corresponding
/// finalizer instances when a particle system is instantiated.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizerFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizerFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetFinalizerType() const = 0;
  virtual void CopyFinalizerProperties(ezParticleFinalizer* pObject, bool bFirstTime) const = 0;

  ezParticleFinalizer* CreateFinalizer(ezParticleSystemInstance* pOwner) const;
};

/// Base class for particle finalizers.
///
/// Finalizers run at the end of each simulation frame to update particle state or
/// perform cleanup operations. They execute after all behaviors have been processed.
/// Examples include updating particle age, applying velocity to position, or computing
/// bounding volumes.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer : public ezParticleModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizer, ezParticleModule);

  friend class ezParticleSystemInstance;

protected:
  ezParticleFinalizer();
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const ezTime& tDiff, ezUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  /// Time delta for the current simulation step.
  ezTime m_TimeDiff;
};
