#pragma once

#include <Foundation/Strings/String.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

struct EZ_PARTICLEPLUGIN_DLL ezParticleRaycastHitReaction
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Bounce,
    Die,
    Stop,

    Default = Bounce
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleRaycastHitReaction);

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Raycast final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Raycast, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Raycast();
  ~ezParticleBehaviorFactory_Raycast();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

  ezEnum<ezParticleRaycastHitReaction> m_Reaction;
  ezUInt8 m_uiCollisionLayer = 0;
  ezString m_sOnCollideEvent;
  float m_fBounceFactor = 0.6f;
  float m_fSizeFactor = 0.1f;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Raycast final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Raycast, ezParticleBehavior);

public:
  ezParticleBehavior_Raycast();

  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;

  ezEnum<ezParticleRaycastHitReaction> m_Reaction;
  ezUInt8 m_uiCollisionLayer = 0;
  ezTempHashedString m_sOnCollideEvent;
  float m_fBounceFactor = 0.6f;
  float m_fSizeFactor = 0.1f;

protected:
  friend class ezParticleBehaviorFactory_Raycast;

  virtual void Process(ezUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule) override;

  ezPhysicsWorldModuleInterface* m_pPhysicsModule;

  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamLastPosition = nullptr;
  ezProcessingStream* m_pStreamVelocity = nullptr;
  const ezProcessingStream* m_pStreamSize = nullptr;
};
