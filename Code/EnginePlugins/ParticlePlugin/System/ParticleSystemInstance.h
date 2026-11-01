#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct ezMsgExtractRenderData;
class ezParticleModule;

/// A particle system stores all data for one 'layer' of a running particle effect
class EZ_PARTICLEPLUGIN_DLL ezParticleSystemInstance
{
public:
  ezParticleSystemInstance();

  void Construct(ezUInt32 uiMaxParticles, ezWorld* pWorld, ezParticleEffectInstance* pOwnerEffect, float fSpawnCountMultiplier);
  void Destruct();

  bool IsVisible() const { return m_bVisible; }

  void SetEmitterEnabled(bool bEnable) { m_bEmitterEnabled = bEnable; }
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void ConfigureFromTemplate(const ezParticleSystemDescriptor* pTemplate);
  void Finalize();

  void ReinitializeStreamProcessors(const ezParticleSystemDescriptor* pTemplate);

  void CreateStreamProcessors(const ezParticleSystemDescriptor* pTemplate);

  void SetupOptionalStreams();

  void SetTransform(const ezTransform& transform, const ezVec3& vParticleStartVelocity);
  const ezTransform& GetTransform() const { return m_Transform; }
  const ezVec3& GetParticleStartVelocity() const { return m_vParticleStartVelocity; }

  ezParticleSystemState::Enum Update(const ezTime& diff);

  ezWorld* GetWorld() const { return m_pWorld; }

  ezUInt64 GetMaxParticles() const { return m_StreamGroup.GetNumElements(); }
  ezUInt64 GetNumActiveParticles() const { return m_StreamGroup.GetNumActiveElements(); }



  /// Returns the desired stream, if it already exists, nullptr otherwise.
  ezProcessingStream* QueryStream(ezTempHashedString sName, ezProcessingStream::DataType type) const;

  /// Returns the desired stream, if it already exists, creates it otherwise.
  void CreateStream(ezStringView sName, ezProcessingStream::DataType type, ezProcessingStream** pStream, ezParticleStreamBinding& ref_binding, bool bExpectInitializedValue);

  void ProcessEventQueue(ezParticleEventQueue queue);

  ezParticleEffectInstance* GetOwnerEffect() const { return m_pOwnerEffect; }
  ezParticleWorldModule* GetOwnerWorldModule() const;

  void ExtractSystemRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const;

  using ParticleDeathHandler = ezEvent<const ezStreamGroupElementRemovedEvent&>::Handler;

  void AddParticleDeathEventHandler(ParticleDeathHandler handler);
  void RemoveParticleDeathEventHandler(ParticleDeathHandler handler);

  void SetBoundingVolume(const ezBoundingBoxSphere& volume, float fMaxParticleSize);
  const ezBoundingBoxSphere& GetBoundingVolume() const { return m_BoundingVolume; }

  bool IsContinuous() const;

  float GetSpawnCountMultiplier() const { return m_fSpawnCountMultiplier; }

  /// Sets the multiplier that the emitters use for their spawn counts. Only has an effect on the next ConfigureFromTemplate().
  ///
  /// fInitializerMultiplier is the part of fMultiplier that comes from ezParticleInitializer::GetSpawnCountMultiplier().
  /// If bInitializerMultiplierChecked is true, the initializers are not asked again, see IsReadyToSpawn().
  void SetSpawnCountMultiplier(float fMultiplier, float fInitializerMultiplier, bool bInitializerMultiplierChecked);

  /// The product of ezParticleInitializer::GetSpawnCountMultiplier() of all initializers, once they were ready.
  float GetRequiredInitializerSpawnCountMultiplier() const { return m_fRequiredInitializerSpawnCountMultiplier; }
  bool IsInitializerSpawnCountMultiplierChecked() const { return m_bInitializerSpawnCountMultiplierChecked; }

  /// Calls ezParticleModule::UpdateFromWorld() on all modules that requested it.
  void UpdateModulesFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject);

  /// Returns false as long as any initializer isn't ready yet, see ezParticleInitializer::IsReadyToSpawn(). Until then no particles are emitted.
  ///
  /// Also returns false, if the initializers require a different spawn count multiplier than the system currently uses, see
  /// ezParticleInitializer::GetSpawnCountMultiplier(). The effect is then reconfigured in the next frame.
  /// This is only checked once, the first time all initializers are ready. A system that gets recreated for this keeps that state, otherwise
  /// a multiplier that keeps changing (e.g. with an animated scale) would recreate the system over and over.
  bool IsReadyToSpawn();

private:
  bool IsEmitterConfigEqual(const ezParticleSystemDescriptor* pTemplate) const;
  bool IsInitializerConfigEqual(const ezParticleSystemDescriptor* pTemplate) const;
  bool IsBehaviorConfigEqual(const ezParticleSystemDescriptor* pTemplate) const;
  bool IsTypeConfigEqual(const ezParticleSystemDescriptor* pTemplate) const;
  bool IsFinalizerConfigEqual(const ezParticleSystemDescriptor* pTemplate) const;

  void CreateStreamZeroInitializers();

  ezSmallArray<ezParticleEmitter*, 1> m_Emitters;
  ezSmallArray<ezParticleInitializer*, 5> m_Initializers;
  ezSmallArray<ezParticleBehavior*, 5> m_Behaviors;
  ezSmallArray<ezParticleFinalizer*, 3> m_Finalizers;
  ezSmallArray<ezParticleType*, 1> m_Types;

  // modules that requested UpdateFromWorld(), collected in Finalize()
  ezSmallArray<ezParticleModule*, 1> m_WorldUpdateModules;

  bool m_bVisible = true; // typically used in editor to hide a system
  bool m_bEmitterEnabled = true;
  bool m_bInitializerSpawnCountMultiplierChecked = false;
  ezParticleEffectInstance* m_pOwnerEffect = nullptr;
  ezWorld* m_pWorld;
  ezTransform m_Transform;
  ezVec3 m_vParticleStartVelocity;
  float m_fSpawnCountMultiplier = 1.0f;
  float m_fInitializerSpawnCountMultiplier = 1.0f;         // the part of m_fSpawnCountMultiplier that comes from the initializers
  float m_fRequiredInitializerSpawnCountMultiplier = 1.0f; // what the initializers want, applied by the next reconfigure of the effect

  ezProcessingStreamGroup m_StreamGroup;

  struct StreamInfo
  {
    ezHashedString m_sName;
    bool m_bGetsInitialized = false;
    bool m_bInUse = false;
    ezProcessingStreamProcessor* m_pDefaultInitializer = nullptr;
  };

  ezSmallArray<StreamInfo, 8> m_StreamInfo;

  // culling data
  ezBoundingBoxSphere m_BoundingVolume;
};
