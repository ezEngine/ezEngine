#pragma once

#include <ParticlePlugin/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Containers/HybridArray.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Reflection/Reflection.h>

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
class EZ_PARTICLEPLUGIN_DLL ezParticleEffectHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezParticleEffectHandle, ezParticleEffectId);
};


struct EZ_PARTICLEPLUGIN_DLL ezParticleSystemState
{
  enum Enum
  {
    Active,
    EmittersFinished,
    OnlyReacting,
    Inactive,
  };
};

class EZ_PARTICLEPLUGIN_DLL ezParticleStreamBinding
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

//////////////////////////////////////////////////////////////////////////

struct EZ_PARTICLEPLUGIN_DLL ezParticleTypeRenderMode
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Additive,
    Blended,
    Opaque,
    Distortion,
    Default = Additive
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleTypeRenderMode);

//////////////////////////////////////////////////////////////////////////

/// \brief What to do when an effect is not visible.
struct EZ_PARTICLEPLUGIN_DLL ezEffectInvisibleUpdateRate
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    FullUpdate,
    Max20fps,
    Max10fps,
    Max5fps,
    Pause,
    Discard,

    Default = Max10fps
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezEffectInvisibleUpdateRate);

//////////////////////////////////////////////////////////////////////////

struct EZ_PARTICLEPLUGIN_DLL ezParticleTextureAtlasType
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None,

    RandomVariations,
    FlipbookAnimation,

    Default = None
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleTextureAtlasType);

//////////////////////////////////////////////////////////////////////////

