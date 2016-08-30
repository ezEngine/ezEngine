#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/HybridArray.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

class ezWorld;
class ezParticleSystemDescriptor;
class ezParticleEmitter;
class ezParticleInitializer;
class ezParticleBehavior;
class ezStreamGroup;
class ezStream;
class ezRandom;
class ezParticleEventQueue;
class ezParticleEffectDescriptor;
class ezParticleWorldModule;
class ezParticleEffectInstance;

typedef ezTypedResourceHandle<class ezParticleEffectResource> ezParticleEffectResourceHandle;

typedef ezGenericId<22, 10> ezParticleEffectId;

/// \brief A handle to a particle effect
class ezParticleEffectHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezParticleEffectHandle, ezParticleEffectId);
};


struct ezParticleSystemState
{
  enum Enum
  {
    Active,
    EmittersFinished,
    OnlyReacting,
    Inactive,
  };
};

class ezParticleStreamBinding
{
public:
  void UpdateBindings(const ezStreamGroup* pGroup) const;
  void Clear() { m_Bindings.Clear(); }

private:
  friend class ezParticleSystemInstance;

  struct Binding
  {
    ezString m_sName;
    ezStream** m_ppStream;
  };

  ezHybridArray<Binding, 4> m_Bindings;
};