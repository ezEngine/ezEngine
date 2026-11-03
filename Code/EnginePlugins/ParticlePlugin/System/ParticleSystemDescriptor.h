#pragma once

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezParticleEmitter;
class ezParticleBehavior;
class ezParticleInitializer;
class ezParticleType;
class ezParticleFinalizer;

class EZ_PARTICLEPLUGIN_DLL ezParticleSystemDescriptor final : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleSystemDescriptor, ezReflectedClass);

public:
  ezParticleSystemDescriptor();
  ~ezParticleSystemDescriptor();

  //////////////////////////////////////////////////////////////////////////
  /// Properties

  const ezHybridArray<ezParticleEmitter*, 1>& GetEmitters() const { return m_Emitters; }

  void AddInitializer(ezParticleInitializer* pModule) { m_Initializers.PushBack(pModule); }
  void RemoveInitializer(ezParticleInitializer* pModule) { m_Initializers.RemoveAndCopy(pModule); }
  const ezHybridArray<ezParticleInitializer*, 4>& GetInitializers() const { return m_Initializers; }

  void AddBehavior(ezParticleBehavior* pModule) { m_Behaviors.PushBack(pModule); }
  void RemoveBehavior(ezParticleBehavior* pModule) { m_Behaviors.RemoveAndCopy(pModule); }
  const ezHybridArray<ezParticleBehavior*, 4>& GetBehaviors() const { return m_Behaviors; }

  void AddType(ezParticleType* pModule) { m_Types.PushBack(pModule); }
  void RemoveType(ezParticleType* pModule) { m_Types.RemoveAndCopy(pModule); }
  const ezHybridArray<ezParticleType*, 2>& GetTypes() const { return m_Types; }

  const ezHybridArray<ezParticleFinalizer*, 2>& GetFinalizers() const { return m_Finalizers; }

  ezTime GetAvgLifetime() const;

  bool m_bVisible;

  ezVarianceTypeTime m_LifeTime;
  ezHashedString m_sOnDeathEvent;
  ezHashedString m_sLifeScaleParameter;

  //////////////////////////////////////////////////////////////////////////

  void Save(ezStreamWriter& inout_stream) const;
  void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor);

  /// Describes the instance data of all modules. Only valid after Load().
  const ezInstanceDataAllocator& GetInstanceDataAllocator() const { return m_InstanceDataAllocator; }

private:
  void ClearEmitters();
  void ClearInitializers();
  void ClearBehaviors();
  void ClearTypes();
  void ClearFinalizers();
  void SetupDefaultProcessors();
  void SetupInstanceData();

  ezString m_sName;
  ezHybridArray<ezParticleEmitter*, 1> m_Emitters;
  ezHybridArray<ezParticleInitializer*, 4> m_Initializers;
  ezHybridArray<ezParticleBehavior*, 4> m_Behaviors;
  ezHybridArray<ezParticleFinalizer*, 2> m_Finalizers;
  ezHybridArray<ezParticleType*, 2> m_Types;
  ezInstanceDataAllocator m_InstanceDataAllocator;
};
