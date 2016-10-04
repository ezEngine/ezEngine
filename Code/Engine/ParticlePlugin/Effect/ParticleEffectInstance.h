#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Math/Transform.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

class ezParticleEffectInstance;

class ezParticleffectUpdateTask : public ezTask
{
public:
  ezParticleffectUpdateTask(ezParticleEffectInstance* pEffect);

  ezTime m_UpdateDiff;

private:
  virtual void Execute() override;

  ezParticleEffectInstance* m_pEffect;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleEffectInstance 
{
  friend class ezParticleWorldModule;

public:
  struct SharedInstance
  {
    const void* m_pSharedInstanceOwner;
    ezTransform m_Transform;
  };

public:
  ezParticleEffectInstance();
  ~ezParticleEffectInstance();

  void Clear();

  void Interrupt();

  const ezParticleEffectHandle& GetHandle() const { return m_hHandle; }

  void SetEmitterEnabled(bool enable);
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void ClearParticleSystems();

  void Configure(const ezParticleEffectResourceHandle& hResource, ezWorld* pWorld, ezParticleWorldModule* pOwnerModule, ezUInt64 uiRandomSeed, bool bIsShared);

  void PreSimulate();


  /// \brief Returns false when the effect is finished
  bool Update(const ezTime& tDiff);

  ezWorld* GetWorld() const { return m_pWorld; }
  ezParticleWorldModule* GetOwnerWorldModule() const { return m_pOwnerModule; }

  const ezParticleEffectResourceHandle& GetResource() const { return m_hResource; }

  const ezHybridArray<ezParticleSystemInstance*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

  bool IsSharedEffect() const { return m_bIsShared; }
  bool IsSimulatedInLocalSpace() const { return m_bSimulateInLocalSpace; }

  void AddSharedInstance(const void* pSharedInstanceOwner);
  void RemoveSharedInstance(const void* pSharedInstanceOwner);
  void SetTransform(const ezTransform& transform, const void* pSharedInstanceOwner = nullptr);
  const ezTransform& GetTransform(const void* pSharedInstanceOwner = nullptr) const;

  const ezDynamicArray<SharedInstance>& GetAllSharedInstances() const { return m_SharedInstances; }

  ezParticleEventQueue* GetEventQueue(const ezTempHashedString& EventType);

  /// \brief Returns the task that is used to update the effect
  ezParticleffectUpdateTask* GetUpdateTask() { return &m_Task; }


private:
  void Reconfigure(ezUInt64 uiRandomSeed, bool bFirstTime);
  void ClearParticleSystem(ezUInt32 index);
  void DestroyEventQueues();
  void ProcessEventQueues();

  ezDynamicArray<SharedInstance> m_SharedInstances;
  ezParticleEffectHandle m_hHandle;
  bool m_bIsShared;
  bool m_bEmitterEnabled;
  bool m_bSimulateInLocalSpace;
  ezUInt8 m_uiReviveTimeout;
  ezTime m_PreSimulateDuration;
  ezParticleEffectResourceHandle m_hResource;

  ezParticleWorldModule* m_pOwnerModule;
  ezWorld* m_pWorld;
  ezTransform m_Transform;
  ezHybridArray<ezParticleSystemInstance*, 4> m_ParticleSystems;

  ezParticleffectUpdateTask m_Task;

  struct EventQueue
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_EventTypeHash;
    ezParticleEventQueue* m_pQueue;
  };

  ezHybridArray<EventQueue, 4> m_EventQueues;
};