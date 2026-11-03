#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Containers/Blob.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

struct ezMsgExtractRenderData;
class ezMessage;

/// Identifies the running particle system that a shared ezParticleModule is called for.
struct ezParticleModuleContext
{
  ezParticleSystemInstance* m_pSystem = nullptr;

  /// The module's part of the system's instance data. Null if ezParticleModule::GetInstanceDataDesc() returned false.
  void* m_pInstanceData = nullptr;

  template <typename T>
  EZ_ALWAYS_INLINE T& GetInstanceData() const
  {
    return *static_cast<T*>(m_pInstanceData);
  }

  ezParticleEffectInstance* GetEffect() const;
  ezRandom& GetRNG() const;

  /// The time step of the current particle system update.
  const ezTime& GetTimeDiff() const;

  /// Gets or creates the stream and stores it in *ppStream, which should point into the instance data.
  void CreateStream(const ezHashedString& sName, ezProcessingStream::DataType type, ezProcessingStream** ppStream, bool bWillInitializeElements) const;
};

/// One module of a running particle system.
struct ezParticleModuleInstance
{
  const ezParticleModule* m_pModule = nullptr;
  ezParticleModuleContext m_Context;
};

/// A particle system stores all data for one 'layer' of a running particle effect
class EZ_PARTICLEPLUGIN_DLL ezParticleSystemInstance
{
public:
  ezParticleSystemInstance();
  ~ezParticleSystemInstance();

  void Construct(ezUInt32 uiMaxParticles, ezWorld* pWorld, ezParticleEffectInstance* pOwnerEffect, float fSpawnCountMultiplier);
  void Destruct();

  bool IsVisible() const { return m_bVisible; }

  void SetEmitterEnabled(bool bEnable) { m_bEmitterEnabled = bEnable; }
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  /// Switches the system over to the modules of the given descriptor.
  ///
  /// If all module types stay the same, the particles, streams and instance data are kept, even if these are new module objects after a resource reload.
  /// Otherwise the instance data is recreated and all particles are lost.
  void ConfigureFromTemplate(const ezParticleSystemDescriptor* pTemplate);

  /// Ends a configuration by calling ezParticleModule::OnFinalize() on all modules.
  ///
  /// Modules may read the transform and the spawn count multiplier in there, so those have to be set before this is called.
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
  ///
  /// The pointer to the stream stays valid until the next ConfigureFromTemplate().
  void CreateStream(const ezHashedString& sName, ezProcessingStream::DataType type, ezProcessingStream** pStream, bool bWillInitializeElements);

  /// Queues the removal of a particle, so this can be called during the update.
  void RemoveParticle(ezUInt64 uiElementIndex) { m_StreamGroup.RemoveElement(uiElementIndex); }

  /// The time step of the current Update().
  const ezTime& GetUpdateTimeDiff() const { return m_UpdateTimeDiff; }

  void ProcessEventQueue(ezParticleEventQueue queue);

  ezParticleEffectInstance* GetOwnerEffect() const { return m_pOwnerEffect; }
  ezParticleWorldModule* GetOwnerWorldModule() const;

  void ExtractSystemRenderData(ezMsgExtractRenderData& ref_msg, const ezTransform& instanceTransform) const;

  void SetBoundingVolume(const ezBoundingBoxSphere& volume, float fMaxParticleSize);
  const ezBoundingBoxSphere& GetBoundingVolume() const { return m_BoundingVolume; }

  bool IsContinuous() const;

  float GetSpawnCountMultiplier() const { return m_fSpawnCountMultiplier; }

  /// Sets the multiplier that the emitters use for their spawn counts. Only has an effect on the next ConfigureFromTemplate().
  ///
  /// fInitializerMultiplier is the part of fMultiplier that comes from ezParticleInitializer::GetInstanceSpawnCountMultiplier().
  /// If bInitializerMultiplierChecked is true, the initializers are not asked again, see PrepareToSpawn().
  void SetSpawnCountMultiplier(float fMultiplier, float fInitializerMultiplier, bool bInitializerMultiplierChecked);

  /// The product of ezParticleInitializer::GetInstanceSpawnCountMultiplier() of all initializers, once they were ready.
  float GetRequiredInitializerSpawnCountMultiplier() const { return m_fRequiredInitializerSpawnCountMultiplier; }
  bool IsInitializerSpawnCountMultiplierChecked() const { return m_bInitializerSpawnCountMultiplierChecked; }

  /// Calls ezParticleModule::UpdateFromWorld() on all modules that requested it.
  void UpdateModulesFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject);

  /// Calls ezParticleModule::OnOwnerMessage() on all modules that requested it.
  void OnOwnerMessage(const ezMessage& msg);

  /// Returns false as long as any initializer isn't ready yet, see ezParticleInitializer::PrepareToSpawn(). Until then no particles are emitted.
  ///
  /// Also returns false, if the initializers require a different spawn count multiplier than the system currently uses, see
  /// ezParticleInitializer::GetInstanceSpawnCountMultiplier(). The effect is then reconfigured in the next frame.
  /// This is only checked once, the first time all initializers are ready. A system that gets recreated for this keeps that state, otherwise
  /// a multiplier that keeps changing (e.g. with an animated scale) would recreate the system over and over.
  bool PrepareToSpawn();

private:
  void CreateStreamDefaultInitializers();

  void CreateModuleInstanceData(const ezParticleSystemDescriptor* pTemplate);
  void DestroyModuleInstanceData();

  /// Calls the function for all modules in the order emitters, initializers, behaviors, finalizers, types.
  template <typename FUNC>
  void ForEachModule(FUNC func);

  void OnElementRemoved(const ezStreamGroupElementRemovedEvent& e);

  ezSmallArray<ezParticleModuleInstance, 1> m_Emitters;
  ezSmallArray<ezParticleModuleInstance, 5> m_Initializers;
  ezSmallArray<ezParticleModuleInstance, 5> m_Behaviors;
  ezSmallArray<ezParticleModuleInstance, 3> m_Finalizers;
  ezSmallArray<ezParticleModuleInstance, 1> m_Types;

  // collected in Finalize()
  ezSmallArray<ezParticleModuleInstance*, 1> m_WorldUpdateModules;
  ezSmallArray<ezParticleModuleInstance*, 1> m_OwnerMessageModules;
  ezSmallArray<ezParticleModuleInstance*, 1> m_ParticleDeathModules;

  /// The template that m_InstanceData was constructed with. The owner effect keeps it alive, even if the resource gets reloaded.
  const ezParticleSystemDescriptor* m_pInstanceDataTemplate = nullptr;
  ezBlob m_InstanceData;
  ezTime m_UpdateTimeDiff;

  bool m_bVisible = true; // typically used in editor to hide a system
  bool m_bEmitterEnabled = true;
  bool m_bInitializerSpawnCountMultiplierChecked = false;
  ezParticleEffectInstance* m_pOwnerEffect = nullptr;
  ezWorld* m_pWorld = nullptr;
  ezTransform m_Transform;
  ezVec3 m_vParticleStartVelocity;
  float m_fSpawnCountMultiplier = 1.0f;
  float m_fInitializerSpawnCountMultiplier = 1.0f;         // the part of m_fSpawnCountMultiplier that comes from the initializers
  float m_fRequiredInitializerSpawnCountMultiplier = 1.0f; // what the initializers want, applied by the next reconfigure of the effect

  ezProcessingStreamGroup m_StreamGroup;
  ezParticleModuleProcessor* m_pModuleProcessor = nullptr; ///< Owned by m_StreamGroup.

  struct StreamInfo
  {
    ezHashedString m_sName;
    bool m_bGetsInitialized = false;
    bool m_bInUse = false;
    bool m_bHasDefaultInitializer = false;
  };

  ezSmallArray<StreamInfo, 8> m_StreamInfo;

  // culling data
  ezBoundingBoxSphere m_BoundingVolume;
};
