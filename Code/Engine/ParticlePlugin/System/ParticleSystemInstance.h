#pragma once

#include <ParticlePlugin/Basics.h>
#include <CoreUtils/DataProcessing/Stream/Stream.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>
#include <Foundation/Math/Random.h>

class ezWorld;
class ezParticleSystemDescriptor;
class ezParticleEmitter;
class ezParticleInitializer;
class ezParticleBehavior;

/// \brief A particle system stores all data for one 'layer' of a running particle effect
class EZ_PARTICLEPLUGIN_DLL ezParticleSystemInstance
{
public:
  ezParticleSystemInstance();
  ~ezParticleSystemInstance();

  void SetEmitterEnabled(bool enable) { m_bEmitterEnabled = enable; }
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void IncreaseRefCount() const { m_RefCount.Increment(); }
  void DecreaseRefCount() const { m_RefCount.Decrement(); }
  ezUInt32 GetRefCount() const { return m_RefCount; }

  void ConfigureFromTemplate(const ezParticleSystemDescriptor* pTemplate);

  void Initialize(ezUInt32 uiMaxParticles, ezWorld* pWorld);
  void SetTransform(const ezTransform& transform) { m_Transform = transform; }
  const ezTransform& GetTransform() const { return m_Transform; }

  void Update();

  ezWorld* GetWorld() const { return m_pWorld; }

  ezUInt64 GetMaxParticles() const { return m_StreamGroup.GetNumElements(); }
  ezUInt64 GetNumActiveParticles() const { return m_StreamGroup.GetNumActiveElements(); }
  const ezStream* GetStreamPosition() const { return m_pStreamPosition; }
  const ezStream* GetStreamVelocity() const { return m_pStreamVelocity; }
  const ezStream* GetStreamColor() const { return m_pStreamColor; }

  ezRandom& GetRNG() { return m_Random; }

private:
  ezHybridArray<ezParticleEmitter*, 2> m_Emitters;
  ezHybridArray<ezParticleInitializer*, 2> m_Initializers;
  ezHybridArray<ezParticleBehavior*, 8> m_Behaviors;

  bool m_bEmitterEnabled;
  ezWorld* m_pWorld;
  ezTransform m_Transform;

  ezStream* m_pStreamPosition;
  ezStream* m_pStreamVelocity;
  ezStream* m_pStreamColor;
  ezStream* m_pStreamLifeTime;
  ezStreamGroup m_StreamGroup;

  ezRandom m_Random;
  mutable ezAtomicInteger32 m_RefCount;
};