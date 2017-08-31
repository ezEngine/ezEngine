#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <Foundation/Types/VarianceTypes.h>

class ezPhysicsWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Volume : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Volume, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Volume();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Volume : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Volume, ezParticleBehavior);
public:
  ezParticleBehavior_Volume();
  ~ezParticleBehavior_Volume();

  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition = nullptr;
  const ezProcessingStream* m_pStreamSize = nullptr;
};
