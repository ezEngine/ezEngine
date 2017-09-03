#pragma once

#include <ParticlePlugin/Basics.h>
#include <Core/World/WorldModule.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <Foundation/Containers/IdTable.h>

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;
class ezParticleEffectInstance;
struct ezResourceEvent;
class ezView;
class ezExtractedRenderData;
class ezTaskGroupID;
class ezParticleStream;
class ezParticleStreamFactory;

/// \brief This world module stores all particle effect data that is active in a given ezWorld instance
///
/// It is used to update all effects in one world and also to render them.
/// When an effect is stopped, it only stops emitting new particles, but it lives on until all particles are dead.
/// Therefore particle effects need to be managed outside of components. When a component dies, it only tells the
/// world module to 'destroy' it's effect, the rest is handled behind the scenes.
class EZ_PARTICLEPLUGIN_DLL ezParticleWorldModule : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();

public:
  ezParticleWorldModule(ezWorld* pWorld);
  ~ezParticleWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  ezParticleEffectHandle CreateEffectInstance(const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, const char* szSharedName /*= nullptr*/, const void*& inout_pSharedInstanceOwner);

  /// \brief This does not actually the effect, it first stops it from emitting and destroys it once all particles have actually died of old age.
  void DestroyEffectInstance(const ezParticleEffectHandle& hEffect, bool bInterruptImmediately, const void* pSharedInstanceOwner);

  bool TryGetEffectInstance(const ezParticleEffectHandle& hEffect, ezParticleEffectInstance*& out_pEffect);

  /// \brief Extracts render data for all effects that are currently active.
  void ExtractRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData) const;

  ezParticleEventQueueManager& GetEventQueueManager() { return m_QueueManager; }

  ezParticleSystemInstance* CreateSystemInstance(ezUInt32 uiMaxParticles, ezWorld* pWorld, ezUInt64 uiRandomSeed, ezParticleEffectInstance* pOwnerEffect);
  void DestroySystemInstance(ezParticleSystemInstance* pInstance);

  ezParticleStream* CreateStreamDefaultInitializer(ezParticleSystemInstance* pOwner, const char* szFullStreamName) const;

private:
  void UpdateEffects(const ezWorldModule::UpdateContext& context);
  void EnsureUpdatesFinished(const ezWorldModule::UpdateContext& context);

  void DestroyFinishedEffects();
  void ResourceEventHandler(const ezResourceEvent& e);
  void ReconfigureEffects();
  ezParticleEffectHandle InternalCreateSharedEffectInstance(const char* szSharedName, const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, const void* pSharedInstanceOwner);
  ezParticleEffectHandle InternalCreateEffectInstance(const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, bool bIsShared);

  void ExtractEffectRenderData(const ezParticleEffectInstance* pEffect, const ezView& view, ezExtractedRenderData* pExtractedRenderData, const ezTransform& systemTransform) const;

  void ConfigureParticleStreamFactories();
  void ClearParticleStreamFactories();

  mutable ezMutex m_Mutex;
  ezDeque<ezParticleEffectInstance> m_ParticleEffects;
  ezDynamicArray<ezParticleEffectInstance*> m_FinishingEffects;
  ezDynamicArray<ezParticleEffectInstance*> m_EffectsToReconfigure;
  ezDynamicArray<ezParticleEffectInstance*> m_ParticleEffectsFreeList;
  ezMap<ezString, ezParticleEffectHandle> m_SharedEffects;
  ezIdTable<ezParticleEffectId, ezParticleEffectInstance*> m_ActiveEffects;
  ezParticleEventQueueManager m_QueueManager;
  mutable ezUInt64 m_uiExtractedFrame; // Increased every frame, passed to modules such that instanced systems can prevent redundant work
  ezDeque<ezParticleSystemInstance> m_ParticleSystems;
  ezDynamicArray<ezParticleSystemInstance*> m_ParticleSystemFreeList;
  ezTaskGroupID m_EffectUpdateTaskGroup;
  ezMap<ezString, ezParticleStreamFactory*> m_StreamFactories;
};

