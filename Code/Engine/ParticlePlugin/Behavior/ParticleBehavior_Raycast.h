#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Raycast : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Raycast, ezParticleBehaviorFactory);

public:
  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

public:

  /// \todo On hit something: bounce, stick, die, send event, spawn prefab, etc.
  /// \todo Collision Filter
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Raycast : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Raycast, ezParticleBehavior);

public:

  virtual void AfterPropertiesConfigured() override;
  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  ezPhysicsWorldModuleInterface* m_pPhysicsModule;

  ezStream* m_pStreamPosition;
  ezStream* m_pStreamVelocity;
};

