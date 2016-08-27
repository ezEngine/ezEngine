#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/HybridArray.h>

class ezWorld;
class ezParticleSystemDescriptor;
class ezParticleEmitter;
class ezParticleInitializer;
class ezParticleBehavior;
class ezStreamGroup;
class ezStream;
class ezRandom;

struct ezParticleSystemState
{
  enum Enum
  {
    Active,
    EmittersFinished,
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