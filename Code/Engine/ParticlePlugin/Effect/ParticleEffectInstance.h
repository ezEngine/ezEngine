#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Math/Transform.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Foundation/Threading/TaskSystem.h>

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
  friend class ezParticleffectUpdateTask;

public:
  struct SharedInstance
  {
    const void* m_pSharedInstanceOwner;
    ezTransform m_Transform;
  };

public:
  ezParticleEffectInstance();
  ~ezParticleEffectInstance();

  void Construct(ezParticleEffectHandle hEffectHandle, const ezParticleEffectResourceHandle& hResource, ezWorld* pWorld, ezParticleWorldModule* pOwnerModule, ezUInt64 uiRandomSeed, bool bIsShared);
  void Destruct();

  void Interrupt();

  const ezParticleEffectHandle& GetHandle() const { return m_hEffectHandle; }

  void SetEmitterEnabled(bool enable);
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void ClearParticleSystems();


  ezWorld* GetWorld() const { return m_pWorld; }
  ezParticleWorldModule* GetOwnerWorldModule() const { return m_pOwnerModule; }

  const ezParticleEffectResourceHandle& GetResource() const { return m_hResource; }

  const ezHybridArray<ezParticleSystemInstance*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

  
  bool IsSimulatedInLocalSpace() const { return m_bSimulateInLocalSpace; }

  void SetTransform(const ezTransform& transform, const void* pSharedInstanceOwner = nullptr);
  const ezTransform& GetTransform(const void* pSharedInstanceOwner = nullptr) const;


  ezParticleEventQueue* GetEventQueue(const ezTempHashedString& EventType);

  /// @name Updates
  /// @{

public:
  /// \brief Returns false when the effect is finished.
  bool Update(const ezTime& tDiff);

private: //friend ezParticleWorldModule
  /// \brief Whether this instance is in a state where its update task should be run
  bool ShouldBeUpdated() const;

  /// \brief Returns the task that is used to update the effect
  ezParticleffectUpdateTask* GetUpdateTask() { return &m_Task; }

private: // friend ezParticleffectUpdateTask
  /// \brief If the effect wants to skip all the initial behavior, this simulates it multiple times before it is shown the first time.
  void PreSimulate();

  /// \brief Applies a given time step, without any restrictions.
  bool StepSimulation(const ezTime& tDiff);

private:
  ezTime m_ElapsedTimeSinceUpdate;


  /// @}

  /// @name Shared Instances
  /// @{
public:

  /// \brief Returns true, if this effect is configured to be simulated once per frame, but rendered by multiple instances.
  bool IsSharedEffect() const { return m_bIsSharedEffect; }

private: // friend ezParticleWorldModule
  void AddSharedInstance(const void* pSharedInstanceOwner);
  void RemoveSharedInstance(const void* pSharedInstanceOwner);


private: //friend ezParticleWorldModule
  const ezDynamicArray<SharedInstance>& GetAllSharedInstances() const { return m_SharedInstances; }

private:
  bool m_bIsSharedEffect = false;

  /// @}

  /// \name Visibility and Culling
  /// @{
public:

  /// \brief Marks this effect as visible from at least one view.
  /// This affects simulation update rates.
  void SetIsVisible() const;

  /// \brief Whether the effect has been marked as visible recently.
  bool IsVisible() const;

  /// \brief Returns true when the last bounding volume update was too long ago.
  bool NeedsBoundingVolumeUpdate() const;

  /// \brief Returns the bounding volume of the effect and the time at which the volume was updated last.
  /// The volume is in the local space of the effect.
  ezTime GetBoundingVolume(ezBoundingBoxSphere& volume) const;

private:
  void CombineSystemBoundingVolumes();

  ezTime m_UpdateBVolumeTime;
  ezTime m_LastBVolumeUpdate;
  ezBoundingBoxSphere m_BoundingVolume;
  mutable ezTime m_EffectIsVisible;
  ezEnum<ezEffectInvisibleUpdateRate> m_InvisibleUpdateRate;

  /// @}



private:
  void Reconfigure(ezUInt64 uiRandomSeed, bool bFirstTime);
  void ClearParticleSystem(ezUInt32 index);
  void DestroyEventQueues();
  void ProcessEventQueues();

  ezDynamicArray<SharedInstance> m_SharedInstances;
  ezParticleEffectHandle m_hEffectHandle;
  bool m_bEmitterEnabled = true;
  bool m_bSimulateInLocalSpace = false;
  bool m_bIsFinishing = false;
  ezUInt8 m_uiReviveTimeout;
  ezInt8 m_iMinSimStepsToDo = 0;
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
