#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Flies : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Flies, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Flies();
  ~ezParticleBehaviorFactory_Flies();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_FinalizerDeps) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  float m_fSpeed = 0.2f;
  float m_fPathLength = 0.2f;
  float m_fMaxEmitterDistance = 0.5f;
  ezAngle m_MaxSteeringAngle = ezAngle::Degree(30);
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Flies : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Flies, ezParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fSpeed = 0.2f;
  float m_fPathLength = 0.2f;
  float m_fMaxEmitterDistance = 0.5f;
  ezAngle m_MaxSteeringAngle = ezAngle::Degree(30);

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamVelocity;

  ezTime m_TimeToChangeDir;
};
