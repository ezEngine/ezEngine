#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>

class ezParticleEmitterFactory;
class ezParticleBehaviorFactory;
class ezParticleInitializerFactory;
class ezParticleTypeFactory;

class EZ_PARTICLEPLUGIN_DLL ezParticleSystemDescriptor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleSystemDescriptor, ezReflectedClass);

public:
  ezParticleSystemDescriptor();
  ~ezParticleSystemDescriptor();

  //////////////////////////////////////////////////////////////////////////
  /// Properties

  const ezHybridArray<ezParticleEmitterFactory*, 2>& GetEmitterFactories() const { return m_EmitterFactories; }

  void AddInitializerFactory(ezParticleInitializerFactory* pFactory) { m_InitializerFactories.PushBack(pFactory); }
  void RemoveInitializerFactory(ezParticleInitializerFactory* pFactory) { m_InitializerFactories.Remove(pFactory); }
  const ezHybridArray<ezParticleInitializerFactory*, 4>& GetInitializerFactories() const { return m_InitializerFactories; }

  void AddBehaviorFactory(ezParticleBehaviorFactory* pFactory) { m_BehaviorFactories.PushBack(pFactory); }
  void RemoveBehaviorFactory(ezParticleBehaviorFactory* pFactory) { m_BehaviorFactories.Remove(pFactory); }
  const ezHybridArray<ezParticleBehaviorFactory*, 8>& GetBehaviorFactories() const { return m_BehaviorFactories; }

  void AddTypeFactory(ezParticleTypeFactory* pFactory) { m_TypeFactories.PushBack(pFactory); }
  void RemoveTypeFactory(ezParticleTypeFactory* pFactory) { m_TypeFactories.Remove(pFactory); }
  const ezHybridArray<ezParticleTypeFactory*, 8>& GetTypeFactories() const { return m_TypeFactories; }

  ezUInt32 m_uiMaxParticles;
  bool m_bVisible;

  ezVarianceTypeTime m_LifeTime;
  ezString m_sOnDeathEvent;

  //////////////////////////////////////////////////////////////////////////

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);

private:
  void ClearEmitters();
  void ClearInitializers();
  void ClearBehaviors();
  void ClearTypes();
  void SetupDefaultProcessors();

  ezString m_sName;
  ezHybridArray<ezParticleEmitterFactory*, 2> m_EmitterFactories;
  ezHybridArray<ezParticleInitializerFactory*, 4> m_InitializerFactories;
  ezHybridArray<ezParticleBehaviorFactory*, 8> m_BehaviorFactories;
  ezHybridArray<ezParticleTypeFactory*, 8> m_TypeFactories;
};
