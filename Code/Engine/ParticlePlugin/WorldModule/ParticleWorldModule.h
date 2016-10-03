#pragma once

#include <ParticlePlugin/Basics.h>
#include <Core/World/WorldModule.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;
class ezParticleEffectInstance;
struct ezResourceEvent;
class ezView;
class ezExtractedRenderData;

/// \brief This world module stores all particle effect data that is active in a given ezWorld instance
///
/// It is used to update all effects in one world and also to render them.
/// When an effect is stopped, it only stops emitting new particles, but it lives on until all particles are dead.
/// Therefore particle effects need to be managed outside of components. When a component dies, it only tells the 
/// world module to 'destroy' it's effect, the rest is handled behind the scenes.
class EZ_PARTICLEPLUGIN_DLL ezParticleWorldModule : public ezWorldModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleWorldModule, ezWorldModule);

public:
  ezParticleWorldModule();

  ezParticleEffectHandle CreateParticleEffectInstance(const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, const char* szSharedName /*= nullptr*/, const void* pSharedInstanceOwner /*= nullptr*/);

  /// \brief This does not actually the effect, it first stops it from emitting and destroys it once all particles have actually died of old age.
  void DestroyParticleEffectInstance(const ezParticleEffectHandle& hEffect, bool bInterruptImmediately, const void* pSharedInstanceOwner);

  bool TryGetEffect(const ezParticleEffectHandle& hEffect, ezParticleEffectInstance*& out_pEffect);

  /// \brief Updates all effects and deallocates those that have been destroyed and are finished.
  void UpdateEffects();

  /// \brief Extracts render data for all effects that are currently active.
  void ExtractRenderData(const ezView& view, ezExtractedRenderData* pExtractedRenderData);

  ezParticleEventQueueManager& GetEventQueueManager() { return m_QueueManager; }

  ezParticleSystemInstance* CreateParticleSystemInstance(ezUInt32 uiMaxParticles, ezWorld* pWorld, ezUInt64 uiRandomSeed, ezParticleEffectInstance* pOwnerEffect);
  void DestroyParticleSystemInstance(ezParticleSystemInstance* pInstance);

private:
  void DestroyFinishedEffects();
  void ResourceEventHandler(const ezResourceEvent& e);
  void ReconfigureEffects();
  ezParticleEffectHandle InternalCreateSharedInstance(const char* szSharedName, const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, const void* pSharedInstanceOwner);
  ezParticleEffectHandle InternalCreateInstance(const ezParticleEffectResourceHandle& hResource, ezUInt64 uiRandomSeed, bool bIsShared);

  ezMutex m_Mutex;
  ezDeque<ezParticleEffectInstance*> m_ParticleEffects;
  ezDeque<ezParticleEffectInstance*> m_FinishingEffects;
  ezDeque<ezParticleEffectInstance*> m_EffectsToReconfigure;
  ezMap<ezString, ezParticleEffectHandle> m_SharedEffects;
  ezIdTable<ezParticleEffectId, ezParticleEffectInstance*> m_ActiveEffects;
  ezParticleEventQueueManager m_QueueManager;
  ezUInt64 m_uiExtractedFrame;
  ezDeque<ezParticleSystemInstance*> m_ParticleSystemFreeList;

protected:
  virtual void InternalStartup() override;
  virtual void InternalBeforeWorldDestruction() override;
  virtual void InternalAfterWorldDestruction() override;
  virtual void InternalUpdate() override;
  virtual void InternalReinit() override {}

};

