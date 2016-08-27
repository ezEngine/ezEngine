#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Math/Transform.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezWorld;
class ezParticleEffectDescriptor;
typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;


typedef ezGenericId<22, 10> ezParticleEffectId;

/// \brief A handle to a particle effect
class ezParticleEffectHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezParticleEffectHandle, ezParticleEffectId);
};


class EZ_PARTICLEPLUGIN_DLL ezParticleEffectInstance 
{
  friend class ezParticleWorldModule;

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

  void Configure(const ezParticleEffectResourceHandle& hResource, ezWorld* pWorld, ezUInt64 uiRandomSeed);

  void SetTransform(const ezTransform& transform);
  const ezTransform& GetTransform() const { return m_Transform; }

  /// \brief Returns false when the effect is finished
  bool Update(const ezTime& tDiff);

  ezWorld* GetWorld() const { return m_pWorld; }

  const ezParticleEffectResourceHandle& GetResource() const { return m_hResource; }

  const ezHybridArray<ezParticleSystemInstance*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

private:
  void Reconfigure(ezUInt64 uiRandomSeed);
  void ClearParticleSystem(ezUInt32 index);

  ezParticleEffectHandle m_hHandle;
  bool m_bEmitterEnabled;
  ezParticleEffectResourceHandle m_hResource;

  ezWorld* m_pWorld;
  ezTransform m_Transform;
  ezHybridArray<ezParticleSystemInstance*, 4> m_ParticleSystems;
};