#pragma once

#include <ParticlePlugin/ParticlePluginDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <ParticlePlugin/Module/ParticleModule.h>

class ezProcessingStream;
class ezParticleSystemInstance;
class ezParticleBehavior;

/// \brief Base class for all particle behaviors
class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetBehaviorType() const = 0;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const = 0;

  ezParticleBehavior* CreateBehavior(ezParticleSystemInstance* pOwner) const;

  virtual void Save(ezStreamWriter& stream) const = 0;
  virtual void Load(ezStreamReader& stream) = 0;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_FinalizerDeps) const {}
};

class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior : public ezParticleModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior, ezParticleModule);

  friend class ezParticleSystemInstance;

protected:
  ezParticleBehavior();
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override {}
  virtual void StepParticleSystem(const ezTime& tDiff, ezUInt32 uiNumNewParticles) { m_TimeDiff = tDiff; }

  ezTime m_TimeDiff;

};


