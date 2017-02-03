#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/HybridArray.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/Id.h>

class ezWorld;
class ezParticleSystemDescriptor;
class ezParticleEmitter;
class ezParticleInitializer;
class ezParticleBehavior;
class ezParticleType;
class ezProcessingStreamGroup;
class ezProcessingStream;
class ezRandom;
class ezParticleEventQueue;
class ezParticleEffectDescriptor;
class ezParticleWorldModule;
class ezParticleEffectInstance;
class ezParticleSystemInstance;
struct ezRenderViewContext;
class ezRenderPipelinePass;

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
  void UpdateBindings(const ezProcessingStreamGroup* pGroup) const;
  void Clear() { m_Bindings.Clear(); }

private:
  friend class ezParticleSystemInstance;

  struct Binding
  {
    ezString m_sName;
    ezProcessingStream** m_ppStream;
  };

  ezHybridArray<Binding, 4> m_Bindings;
};
