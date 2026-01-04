#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/RefCounted.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezWorld;
class ezParticleSystemDescriptor;
class ezParticleEventReactionFactory;
class ezParticleEventReaction;
class ezParticleEmitter;
class ezParticleInitializer;
class ezParticleBehavior;
class ezParticleType;
class ezProcessingStreamGroup;
class ezProcessingStream;
class ezRandom;
struct ezParticleEvent;
class ezParticleEffectDescriptor;
class ezParticleWorldModule;
class ezParticleEffectInstance;
class ezParticleSystemInstance;
struct ezRenderViewContext;
class ezRenderPipelinePass;
class ezParticleFinalizer;
class ezParticleFinalizerFactory;

using ezParticleEffectResourceHandle = ezTypedResourceHandle<class ezParticleEffectResource>;

using ezParticleEffectId = ezGenericId<22, 10>;

/// Handle to a particle effect instance
class EZ_PARTICLEPLUGIN_DLL ezParticleEffectHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezParticleEffectHandle, ezParticleEffectId);
};


/// Current state of a particle system
struct EZ_PARTICLEPLUGIN_DLL ezParticleSystemState
{
  enum Enum
  {
    Active,           ///< System is actively emitting and processing particles
    EmittersFinished, ///< Emitters stopped, particles still alive
    OnlyReacting,     ///< Only reacting to events, otherwise finished
    Inactive,         ///< System is inactive
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

/// Blending mode for particle rendering
struct EZ_PARTICLEPLUGIN_DLL ezParticleTypeRenderMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Additive,          ///< Additive blending
    Blended,           ///< Alpha blending
    Opaque,            ///< Opaque rendering
    Unused,
    BlendedBackground, ///< Alpha blending in background pass
    BlendedForeground, ///< Alpha blending in foreground pass
    Unused2,
    Default = Additive
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleTypeRenderMode);

//////////////////////////////////////////////////////////////////////////

/// Lighting mode for particles
struct EZ_PARTICLEPLUGIN_DLL ezParticleLightingMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Fullbright, ///< No lighting applied
    VertexLit,  ///< Simple vertex lighting
    Default = Fullbright
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleLightingMode);

//////////////////////////////////////////////////////////////////////////

/// Update rate for effects that are not visible
struct EZ_PARTICLEPLUGIN_DLL ezEffectInvisibleUpdateRate
{
  using StorageType = ezUInt8;

  enum Enum
  {
    FullUpdate, ///< Continue updating at full rate
    Max20fps,   ///< Update at most 20 times per second
    Max10fps,   ///< Update at most 10 times per second
    Max5fps,    ///< Update at most 5 times per second
    Pause,      ///< Pause simulation completely
    Discard,    ///< Delete the effect

    Default = Max10fps
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezEffectInvisibleUpdateRate);

//////////////////////////////////////////////////////////////////////////

/// How to use texture atlas for particle sprites
struct EZ_PARTICLEPLUGIN_DLL ezParticleTextureAtlasType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None,              ///< No atlas, use full texture

    RandomVariations,  ///< Pick random tile for variation
    FlipbookAnimation, ///< Animate through tiles over lifetime
    RandomYAnimatedX,  ///< Random Y row, animate X over lifetime

    Default = None
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleTextureAtlasType);

//////////////////////////////////////////////////////////////////////////

/// How to sample color gradients for particles
struct EZ_PARTICLEPLUGIN_DLL ezParticleColorGradientMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Age,   ///< Sample based on normalized particle lifetime (0-1)
    Speed, ///< Sample based on particle speed

    Default = Age
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleColorGradientMode);

//////////////////////////////////////////////////////////////////////////

/// Source of curve data
struct EZ_PARTICLEPLUGIN_DLL ezCurveSource
{
  using StorageType = ezUInt8;

  enum Enum
  {
    CustomCurve, ///< Use embedded curve data
    SharedCurve, ///< Reference shared curve resource

    Default = CustomCurve
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezCurveSource);

//////////////////////////////////////////////////////////////////////////

/// Source of gradient data
struct EZ_PARTICLEPLUGIN_DLL ezGradientSource
{
  using StorageType = ezUInt8;

  enum Enum
  {
    CustomGradient, ///< Use embedded gradient data
    SharedGradient, ///< Reference shared gradient resource

    Default = CustomGradient
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezGradientSource);

//////////////////////////////////////////////////////////////////////////

/// Action when particles leave bounds
struct EZ_PARTICLEPLUGIN_DLL ezParticleOutOfBoundsMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Teleport, ///< Wrap particle to opposite side
    Die,      ///< Kill the particle

    Default = Teleport
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleOutOfBoundsMode);

//////////////////////////////////////////////////////////////////////////

struct ezParticleEffectFloatParam
{
  EZ_DECLARE_POD_TYPE();
  ezHashedString m_sName;
  float m_Value;
};

struct ezParticleEffectColorParam
{
  EZ_DECLARE_POD_TYPE();
  ezHashedString m_sName;
  ezColor m_Value;
};

class ezParticleEffectParameters final : public ezRefCounted
{
public:
  ezHybridArray<ezParticleEffectFloatParam, 2> m_FloatParams;
  ezHybridArray<ezParticleEffectColorParam, 2> m_ColorParams;
};
