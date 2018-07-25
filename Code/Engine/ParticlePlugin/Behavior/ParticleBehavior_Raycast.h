#pragma once

#include <Foundation/Strings/String.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

struct EZ_PARTICLEPLUGIN_DLL ezParticleRaycastHitReaction
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Bounce,
    Die,
    Stop,

    Default = Bounce
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleRaycastHitReaction);

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Raycast : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Raycast, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Raycast();
  ~ezParticleBehaviorFactory_Raycast();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject) const override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_FinalizerDeps) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezEnum<ezParticleRaycastHitReaction> m_Reaction;
  ezUInt8 m_uiCollisionLayer = 0;
  ezString m_sOnCollideEvent;
  float m_fBounceFactor = 0.6f;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Raycast : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Raycast, ezParticleBehavior);

public:
  virtual void AfterPropertiesConfigured(bool bFirstTime) override;
  virtual void CreateRequiredStreams() override;

  ezEnum<ezParticleRaycastHitReaction> m_Reaction;
  ezUInt8 m_uiCollisionLayer = 0;
  ezTempHashedString m_sOnCollideEvent;
  float m_fBounceFactor = 0.6f;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  ezPhysicsWorldModuleInterface* m_pPhysicsModule;

  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamLastPosition = nullptr;
  ezProcessingStream* m_pStreamVelocity = nullptr;
};
