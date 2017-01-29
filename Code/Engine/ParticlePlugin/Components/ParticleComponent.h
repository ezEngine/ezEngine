#pragma once

#include <ParticlePlugin/Basics.h>
#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/ResourceManager/ResourceBase.h>
#include <Core/World/ComponentManager.h>
#include <RendererCore/Components/RenderComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>

class ezParticleRenderData;
struct ezUpdateLocalBoundsMessage;
struct ezExtractRenderDataMessage;
class ezParticleSystemInstance;
class ezParticleComponent;

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

typedef ezComponentManagerSimple<ezParticleComponent, ezComponentUpdateType::WhenSimulating> ezParticleComponentManager;


class EZ_PARTICLEPLUGIN_DLL ezParticleComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezParticleComponent, ezRenderComponent, ezParticleComponentManager);

public:
  ezParticleComponent();
  ~ezParticleComponent();

  void Update();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  /// \brief Returns true, if a new effect was spawned.
  bool SpawnEffect();
  /// \brief Stops emitting further particles.
  void StopEffect();
  /// \brief Cancels the entire effect immediately, it will pop out of existence.
  void InterruptEffect();
  /// \brief Returns true, if an effect is currently in a state where it might emit new particles
  bool IsEffectActive() const;

  // ************************************* PROPERTIES ***********************************

  void SetParticleEffect(const ezParticleEffectResourceHandle& hEffect);
  EZ_FORCE_INLINE const ezParticleEffectResourceHandle& GetParticleEffect() const { return m_hEffectResource; }

  void SetParticleEffectFile(const char* szFile);
  const char* GetParticleEffectFile() const;

  ezUInt64 m_uiRandomSeed;
  ezString m_sSharedInstanceName;

  bool m_bSpawnAtStart;
  bool m_bAutoRestart;
  ezTime m_MinRestartDelay;
  ezTime m_RestartDelayRange;

  //////////////////////////////////////////////////////////////////////////


  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds) override;

  ezParticleEffectController m_EffectController;

protected:
  ezParticleEffectResourceHandle m_hEffectResource;
  ezTime m_RestartTime;

  virtual void OnBeforeDetachedFromObject() override;

};
