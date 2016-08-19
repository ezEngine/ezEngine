#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>
#include <CoreUtils/DataProcessing/Stream/StreamProcessor.h>

class ezStream;
class ezParticleSystemInstance;
class ezParticleBehavior;

/// \brief Base class for all particle behaviors
class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory, ezReflectedClass);

public:
  ezParticleBehaviorFactory();

  virtual ezParticleBehavior* CreateBehavior(ezParticleSystemInstance* pOwner) const = 0;

  virtual void Save(ezStreamWriter& stream) const = 0;
  virtual void Load(ezStreamReader& stream) = 0;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior : public ezStreamProcessor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior, ezStreamProcessor);

  friend class ezParticleSystemInstance;

protected:
  ezParticleBehavior(ezParticleSystemInstance* pOwner);

  virtual ezResult UpdateStreamBindings() override;
  virtual void StepParticleSystem(const ezTime& tDiff) { m_TimeDiff = tDiff; }

  ezTime m_TimeDiff;
  ezParticleSystemInstance* m_pParticleSystem;
  ezStream* m_pStreamPosition;
  ezStream* m_pStreamVelocity;
  ezStream* m_pStreamColor;
  ezStream* m_pStreamLifeTime;
};


