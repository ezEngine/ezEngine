#pragma once

#include <ParticlePlugin/ParticlePluginDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <ParticlePlugin/Module/ParticleModule.h>

class ezProcessingStream;
class ezParticleSystemInstance;
class ezParticleFinalizer;

/// \brief Base class for all particle Finalizers
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizerFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizerFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetFinalizerType() const = 0;
  virtual void CopyFinalizerProperties(ezParticleFinalizer* pObject, bool bFirstTime) const = 0;

  ezParticleFinalizer* CreateFinalizer(ezParticleSystemInstance* pOwner) const;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer : public ezParticleModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizer, ezParticleModule);

  friend class ezParticleSystemInstance;

protected:
  ezParticleFinalizer();
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const ezTime& tDiff, ezUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  ezTime m_TimeDiff;

};


