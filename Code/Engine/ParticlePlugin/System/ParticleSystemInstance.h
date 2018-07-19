#pragma once

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/Basics.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/Events/ParticleEvent.h>

class ezView;
class ezExtractedRenderData;

/// \brief A particle system stores all data for one 'layer' of a running particle effect
class EZ_PARTICLEPLUGIN_DLL ezParticleSystemInstance
{
public:
  ezParticleSystemInstance();

  void Construct(ezUInt32 uiMaxParticles, ezWorld* pWorld, ezUInt64 uiRandomSeed, ezParticleEffectInstance* pOwnerEffect);
  void Destruct();

  bool IsVisible() const { return m_bVisible; }

  void SetEmitterEnabled(bool enable) { m_bEmitterEnabled = enable; }
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

  ezParticleSystemState::Enum Update(const ezTime& tDiff);

  ezWorld* GetWorld() const { return m_pWorld; }

  ezUInt64 GetMaxParticles() const { return m_StreamGroup.GetNumElements(); }
  ezUInt64 GetNumActiveParticles() const { return m_StreamGroup.GetNumActiveElements(); }

  ezRandom& GetRNG() { return m_Random; }

  /// \brief Returns the desired stream, if it already exists, nullptr otherwise.
  const ezProcessingStream* QueryStream(const char* szName, ezProcessingStream::DataType Type) const;

  /// \brief Returns the desired stream, if it already exists, creates it otherwise.
  void CreateStream(const char* szName, ezProcessingStream::DataType Type, ezProcessingStream** ppStream, ezParticleStreamBinding& binding,
                    bool bExpectInitializedValue);

  void ProcessEventQueue(ezParticleEventQueue queue);

  ezParticleEffectInstance* GetOwnerEffect() const { return m_pOwnerEffect; }
  ezParticleWorldModule* GetOwnerWorldModule() const;

  void ExtractSystemRenderData(const ezView& view, ezExtractedRenderData& extractedRenderData, const ezTransform& instanceTransform,
                               ezUInt64 uiExtractedFrame) const;

  typedef ezEvent<const ezStreamGroupElementRemovedEvent&>::Handler ParticleDeathHandler;

  void AddParticleDeathEventHandler(ParticleDeathHandler handler);
  void RemoveParticleDeathEventHandler(ParticleDeathHandler handler);

  void SetBoundingVolume(const ezBoundingBoxSphere& volume, float fMaxParticleSize);
  void GetBoundingVolume(ezBoundingBoxSphere& volume) const;

  bool IsContinuous() const;

private:
  bool IsEmitterConfigEqual(const ezParticleSystemDescriptor* pTemplate) const;
  bool IsInitializerConfigEqual(const ezParticleSystemDescriptor* pTemplate) const;
  bool IsBehaviorConfigEqual(const ezParticleSystemDescriptor* pTemplate) const;
  bool IsTypeConfigEqual(const ezParticleSystemDescriptor* pTemplate) const;

  void CreateStreamZeroInitializers();

  ezHybridArray<ezParticleEmitter*, 2> m_Emitters;
  ezHybridArray<ezParticleInitializer*, 6> m_Initializers;
  ezHybridArray<ezParticleBehavior*, 6> m_Behaviors;
  ezHybridArray<ezParticleType*, 2> m_Types;

  bool m_bVisible; // typically used in editor to hide a system
  bool m_bEmitterEnabled;
  ezParticleEffectInstance* m_pOwnerEffect;
  ezWorld* m_pWorld;
  ezTransform m_Transform;
  ezVec3 m_vParticleStartVelocity;

  ezProcessingStreamGroup m_StreamGroup;

  struct StreamInfo
  {
    ezString m_sName;
    bool m_bGetsInitialized = false;
    bool m_bInUse = false;
    ezProcessingStreamProcessor* m_pDefaultInitializer = nullptr;
  };

  ezHybridArray<StreamInfo, 16> m_StreamInfo;

  ezRandom m_Random;

  // culling data
  ezBoundingBoxSphere m_BoundingVolume;
};
