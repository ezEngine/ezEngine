#pragma once

#include <ParticlePlugin/ParticlePluginDLL.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>

class ezParticleEmitterFactory;
class ezParticleBehaviorFactory;
class ezParticleInitializerFactory;
class ezParticleTypeFactory;

class EZ_PARTICLEPLUGIN_DLL ezParticleSystemDescriptor final : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleSystemDescriptor, ezReflectedClass);

public:
  ezParticleSystemDescriptor();
  ~ezParticleSystemDescriptor();

  //////////////////////////////////////////////////////////////////////////
  /// Properties

  const ezHybridArray<ezParticleEmitterFactory*, 1>& GetEmitterFactories() const { return m_EmitterFactories; }

  void AddInitializerFactory(ezParticleInitializerFactory* pFactory) { m_InitializerFactories.PushBack(pFactory); }
  void RemoveInitializerFactory(ezParticleInitializerFactory* pFactory) { m_InitializerFactories.RemoveAndCopy(pFactory); }
  const ezHybridArray<ezParticleInitializerFactory*, 4>& GetInitializerFactories() const { return m_InitializerFactories; }

  void AddBehaviorFactory(ezParticleBehaviorFactory* pFactory) { m_BehaviorFactories.PushBack(pFactory); }
  void RemoveBehaviorFactory(ezParticleBehaviorFactory* pFactory) { m_BehaviorFactories.RemoveAndCopy(pFactory); }
  const ezHybridArray<ezParticleBehaviorFactory*, 4>& GetBehaviorFactories() const { return m_BehaviorFactories; }

  void AddTypeFactory(ezParticleTypeFactory* pFactory) { m_TypeFactories.PushBack(pFactory); }
  void RemoveTypeFactory(ezParticleTypeFactory* pFactory) { m_TypeFactories.RemoveAndCopy(pFactory); }
  const ezHybridArray<ezParticleTypeFactory*, 2>& GetTypeFactories() const { return m_TypeFactories; }

  const ezHybridArray<ezParticleFinalizerFactory*, 2>& GetFinalizerFactories() const { return m_FinalizerFactories; }

  ezTime GetAvgLifetime() const;

  bool m_bVisible;

  ezVarianceTypeTime m_LifeTime;
  ezString m_sOnDeathEvent;
  ezString m_sLifeScaleParameter;

  //////////////////////////////////////////////////////////////////////////

  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);

private:
  void ClearEmitters();
  void ClearInitializers();
  void ClearBehaviors();
  void ClearTypes();
  void ClearFinalizers();
  void SetupDefaultProcessors();

  ezString m_sName;
  ezHybridArray<ezParticleEmitterFactory*, 1> m_EmitterFactories;
  ezHybridArray<ezParticleInitializerFactory*, 4> m_InitializerFactories;
  ezHybridArray<ezParticleBehaviorFactory*, 4> m_BehaviorFactories;
  ezHybridArray<ezParticleFinalizerFactory*, 2> m_FinalizerFactories;
  ezHybridArray<ezParticleTypeFactory*, 2> m_TypeFactories;
};
