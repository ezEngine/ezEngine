#pragma once

#include <Foundation/Math/Transform.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/SharedPtr.h>

#include <ParticlePlugin/System/ParticleSystemInstance.h>


class ezParticleEffectInstance;

class ezParticleEffectUpdateTask final : public ezTask
{
public:
  ezParticleEffectUpdateTask(ezParticleEffectInstance* pEffect);

  ezTime m_UpdateDiff;

private:
  virtual void Execute() override;

  ezParticleEffectInstance* m_pEffect;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleEffectInstance
{
  friend class ezParticleWorldModule;
  friend class ezParticleEffectUpdateTask;

public:
  ezParticleEffectInstance();
  ~ezParticleEffectInstance();

  void Construct(ezParticleEffectHandle hEffectHandle, const ezParticleEffectResourceHandle& hResource, ezWorld* pWorld, ezParticleWorldModule* pOwnerModule, ezUInt64 uiRandomSeed, bool bIsShared, ezArrayPtr<ezParticleEffectFloatParam> floatParams, ezArrayPtr<ezParticleEffectColorParam> colorParams);
  void Destruct();

  void Interrupt();

  const ezParticleEffectHandle& GetHandle() const { return m_hEffectHandle; }

  void SetEmitterEnabled(bool bEnable);
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void ClearParticleSystems();
  void ClearEventReactions();

  bool IsContinuous() const;

  ezWorld* GetWorld() const { return m_pWorld; }
  ezParticleWorldModule* GetOwnerWorldModule() const { return m_pOwnerModule; }

  const ezParticleEffectResourceHandle& GetResource() const { return m_hResource; }

  const ezHybridArray<ezParticleSystemInstance*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

  void AddParticleEvent(const ezParticleEvent& pe);

  ezRandom& GetRNG() { return m_Random; }

  ezUInt64 GetRandomSeed() const { return m_uiRandomSeed; }

  void RequestWindSamples();
  void UpdateWindSamples(ezTime tDiff);

  /// \brief Returns the number of currently active particles across all systems.
  ezUInt64 GetNumActiveParticles() const;

  /// @name Transform Related
  /// @{
public:
  /// \brief Whether the effect is simulated around the origin and thus not affected by instance position and rotation
  bool IsSimulatedInLocalSpace() const { return m_bSimulateInLocalSpace; }

  /// \brief Sets the transformation of this instance
  void SetTransform(const ezTransform& transform, const ezVec3& vParticleStartVelocity);

  /// \brief Sets the transformation of this instance that should be used next frame.
  /// This function is typically used to set the transformation while the particle simulation is running to prevent race conditions.
  void SetTransformForNextFrame(const ezTransform& transform, const ezVec3& vParticleStartVelocity);

  /// \brief Returns the transform of the main or shared instance.
  const ezTransform& GetTransform() const { return m_Transform; }

  /// \brief For the renderer to know whether the instance transform has to be applied to each particle position.
  bool NeedsToApplyTransform() const { return m_bSimulateInLocalSpace || m_bIsSharedEffect; }

  /// \brief Returns the wind at the given position.
  ///
  /// Returns a zero vector, if no wind value is available (invalid index).
  ezSimdVec4f GetWindAt(const ezSimdVec4f& position) const;

private:
  void PassTransformToSystems();

  ezTransform m_Transform;
  ezTransform m_TransformForNextFrame;

  ezVec3 m_vVelocity;
  ezVec3 m_vVelocityForNextFrame;

  struct WindSampleGrid
  {
    ezSimdVec4f m_vMinPos;
    ezSimdVec4f m_vMaxPos;
    ezSimdVec4f m_vInvCellSize;
    ezSmallArray<ezSimdVec4f, 16, ezAlignedAllocatorWrapper> m_Samples;
  };

  ezVec4I32 m_vNumWindSamples; // not unsigned so it can be loaded directly to a SIMD register, w contains total num samples
  mutable ezUniquePtr<WindSampleGrid> m_WindSampleGrids[2];

  /// @}
  /// @name Updates
  /// @{

public:
  /// \brief Returns false when the effect is finished.
  bool Update(const ezTime& diff);

  /// \brief Returns the total (game) time that the effect is alive and has been updated.
  ///
  /// Use this time, instead of a world clock, for time-dependent calculations. It is mostly tied to the world clock (game update),
  /// but additionally includes pre-simulation timings, which would otherwise be left out which can break some calculations.
  ezTime GetTotalEffectLifeTime() const { return m_TotalEffectLifeTime; }

private: // friend ezParticleWorldModule
  /// \brief Whether this instance is in a state where its update task should be run
  bool ShouldBeUpdated() const;

  /// \brief Returns the task that is used to update the effect
  const ezSharedPtr<ezTask>& GetUpdateTask() { return m_pTask; }

private: // friend ezParticleEffectUpdateTask
  friend class ezParticleEffectController;
  /// \brief If the effect wants to skip all the initial behavior, this simulates it multiple times before it is shown the first time.
  void PreSimulate();

  /// \brief Applies a given time step, without any restrictions.
  bool StepSimulation(const ezTime& tDiff);

private:
  ezTime m_TotalEffectLifeTime = ezTime::MakeZero();
  ezTime m_ElapsedTimeSinceUpdate = ezTime::MakeZero();


  /// @}
  /// @name Shared Instances
  /// @{
public:
  /// \brief Returns true, if this effect is configured to be simulated once per frame, but rendered by multiple instances.
  bool IsSharedEffect() const { return m_bIsSharedEffect; }

private: // friend ezParticleWorldModule
  void AddSharedInstance(const void* pSharedInstanceOwner);
  void RemoveSharedInstance(const void* pSharedInstanceOwner);

private:
  bool m_bIsSharedEffect = false;

  /// @}
  /// \name Visibility and Culling
  /// @{
public:
  /// \brief Marks this effect as visible from at least one view.
  /// This affects simulation update rates.
  void SetIsVisible() const;

  void SetVisibleIf(ezParticleEffectInstance* pOtherVisible);

  /// \brief Whether the effect has been marked as visible recently.
  bool IsVisible() const;

  /// \brief Returns the bounding volume of the effect.
  /// The volume is in the local space of the effect.
  void GetBoundingVolume(ezBoundingBoxSphere& ref_volume) const;

private:
  void CombineSystemBoundingVolumes();

  ezBoundingBoxSphere m_BoundingVolume;
  mutable ezTime m_EffectIsVisible;
  ezParticleEffectInstance* m_pVisibleIf = nullptr;
  ezEnum<ezEffectInvisibleUpdateRate> m_InvisibleUpdateRate;
  ezUInt64 m_uiRandomSeed = 0;

  /// @}
  /// \name Effect Parameters
  /// @{
public:
  void SetParameter(const ezTempHashedString& sName, float value);
  void SetParameter(const ezTempHashedString& sName, const ezColor& value);

  ezInt32 FindFloatParameter(const ezTempHashedString& sName) const;
  float GetFloatParameter(const ezTempHashedString& sName, float fDefaultValue) const;
  float GetFloatParameter(ezUInt32 uiIdx) const { return m_FloatParameters[uiIdx].m_fValue; }

  ezInt32 FindColorParameter(const ezTempHashedString& sName) const;
  const ezColor& GetColorParameter(const ezTempHashedString& sName, const ezColor& defaultValue) const;
  const ezColor& GetColorParameter(ezUInt32 uiIdx) const { return m_ColorParameters[uiIdx].m_Value; }


private:
  struct FloatParameter
  {
    EZ_DECLARE_POD_TYPE();
    ezUInt64 m_uiNameHash;
    float m_fValue;
  };

  struct ColorParameter
  {
    EZ_DECLARE_POD_TYPE();
    ezUInt64 m_uiNameHash;
    ezColor m_Value;
  };

  ezHybridArray<FloatParameter, 2> m_FloatParameters;
  ezHybridArray<ColorParameter, 2> m_ColorParameters;

  /// @}


private:
  void Reconfigure(bool bFirstTime, ezArrayPtr<ezParticleEffectFloatParam> floatParams, ezArrayPtr<ezParticleEffectColorParam> colorParams);
  void ClearParticleSystem(ezUInt32 index);
  void ProcessEventQueues();

  // for deterministic randomness
  ezRandom m_Random;

  ezHashSet<const void*> m_SharedInstances;
  ezParticleEffectHandle m_hEffectHandle;
  bool m_bEmitterEnabled = true;
  bool m_bSimulateInLocalSpace = false;
  bool m_bIsFinishing = false;
  ezUInt8 m_uiReviveTimeout = 3;
  ezInt8 m_iMinSimStepsToDo = 0;
  float m_fApplyInstanceVelocity = 0;
  ezTime m_PreSimulateDuration;
  ezParticleEffectResourceHandle m_hResource;

  ezParticleWorldModule* m_pOwnerModule = nullptr;
  ezWorld* m_pWorld = nullptr;
  ezHybridArray<ezParticleSystemInstance*, 4> m_ParticleSystems;
  ezHybridArray<ezParticleEventReaction*, 4> m_EventReactions;

  ezSharedPtr<ezTask> m_pTask;

  ezStaticArray<ezParticleEvent, 16> m_EventQueue;
};
