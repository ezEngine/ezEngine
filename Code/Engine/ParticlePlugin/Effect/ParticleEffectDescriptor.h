#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

class ezParticleSystemDescriptor;

class EZ_PARTICLEPLUGIN_DLL ezParticleEffectDescriptor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEffectDescriptor, ezReflectedClass);

public:
  ezParticleEffectDescriptor();
  ~ezParticleEffectDescriptor();

  void AddParticleSystem(ezParticleSystemDescriptor* pSystem) { m_ParticleSystems.PushBack(pSystem); }
  void RemoveParticleSystem(ezParticleSystemDescriptor* pSystem) { m_ParticleSystems.Remove(pSystem); }
  const ezHybridArray<ezParticleSystemDescriptor*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);

  void ClearSystems();

private:
  ezHybridArray<ezParticleSystemDescriptor*, 4> m_ParticleSystems;
};