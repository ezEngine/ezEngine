#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Math/Transform.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezWorld;
class ezParticleEffectDescriptor;
typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleEffectInstance 
{
public:
  ezParticleEffectInstance();
  ~ezParticleEffectInstance();

  void SetEmitterEnabled(bool enable);
  bool GetEmitterEnabled() const { return m_bEmitterEnabled; }

  bool HasActiveParticles() const;

  void ClearParticleSystems();

  void Configure(const ezParticleEffectResourceHandle& hResource, ezWorld* pWorld);
  void Reconfigure();

  void SetTransform(const ezTransform& transform);
  const ezTransform& GetTransform() const { return m_Transform; }

  void Update();

  ezWorld* GetWorld() const { return m_pWorld; }

  const ezParticleEffectResourceHandle& GetResource() const { return m_hResource; }

  const ezHybridArray<ezParticleSystemInstance*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

private:
  void ClearParticleSystem(ezUInt32 index);

  bool m_bEmitterEnabled;
  ezParticleEffectResourceHandle m_hResource;

  ezWorld* m_pWorld;
  ezTransform m_Transform;
  ezHybridArray<ezParticleSystemInstance*, 4> m_ParticleSystems;
};