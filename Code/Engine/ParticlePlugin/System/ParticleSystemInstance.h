#pragma once

#include <ParticlePlugin/Basics.h>
#include <CoreUtils/DataProcessing/Stream/Stream.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>
#include <Foundation/Math/Random.h>
#include <ParticlePlugin/Declarations.h>

/// \brief A particle system stores all data for one 'layer' of a running particle effect
class EZ_PARTICLEPLUGIN_DLL ezParticleSystemInstance
{
public:
  ezParticleSystemInstance();

  mutable ezMutex m_Mutex;

  bool IsVisible() const { return m_bVisible; }

  void SetEmitterEnabled(bool enable) { m_bEmitterEnabled = enable; }
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void IncreaseRefCount() const { m_RefCount.Increment(); }
  void DecreaseRefCount() const { m_RefCount.Decrement(); }
  ezUInt32 GetRefCount() const { return m_RefCount; }

  void ConfigureFromTemplate(const ezParticleSystemDescriptor* pTemplate);

  void Initialize(ezUInt32 uiMaxParticles, ezWorld* pWorld, ezUInt64 uiRandomSeed);
  void SetTransform(const ezTransform& transform) { m_Transform = transform; }
  const ezTransform& GetTransform() const { return m_Transform; }

  ezParticleSystemState::Enum Update(const ezTime& tDiff);

  ezWorld* GetWorld() const { return m_pWorld; }

  ezUInt64 GetMaxParticles() const { return m_StreamGroup.GetNumElements(); }
  ezUInt64 GetNumActiveParticles() const { return m_StreamGroup.GetNumActiveElements(); }

  ezRandom& GetRNG() { return m_Random; }

  /// \brief Returns the desired stream, if it already exists, nullptr otherwise.
  const ezStream* QueryStream(const char* szName, ezStream::DataType Type) const;

  /// \brief Returns the desired stream, if it already exists, creates it otherwise.
  void CreateStream(const char* szName, ezStream::DataType Type, ezStream** ppStream, ezParticleStreamBinding& binding, bool bExpectInitializedValue);

private:
  void CreateStreamZeroInitializers();

  ezHybridArray<ezParticleEmitter*, 2> m_Emitters;
  ezHybridArray<ezParticleInitializer*, 2> m_Initializers;
  ezHybridArray<ezParticleBehavior*, 8> m_Behaviors;

  bool m_bVisible; // typically used in editor to hide a system
  bool m_bEmitterEnabled;
  ezWorld* m_pWorld;
  ezTransform m_Transform;

  ezStreamGroup m_StreamGroup;

  struct StreamInfo
  {
    StreamInfo();

    ezString m_sName;
    bool m_bGetsInitialized;
    bool m_bInUse;
    ezStreamElementSpawner* m_pInitializer;
  };

  ezHybridArray<StreamInfo, 8> m_StreamInfo;

  ezRandom m_Random;
  mutable ezAtomicInteger32 m_RefCount;
};