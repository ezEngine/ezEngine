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
class ezParticleEventReaction;
class ezProcessingStreamGroup;
class ezProcessingStream;
class ezRandom;
struct ezParticleEvent;
class ezParticleEffectDescriptor;
class ezParticleWorldModule;
class ezParticleEffectInstance;
class ezParticleSystemInstance;
class ezRenderViewContext;
class ezRenderPipelinePass;
class ezParticleModule;
class ezParticleModuleProcessor;
class ezParticleStream;
struct ezParticleEffectResourceDescriptor;
class ezParticleFinalizer;

using ezParticleEffectResourceHandle = ezTypedResourceHandle<class ezParticleEffectResource>;

using ezParticleEffectId = ezGenericId<22, 10>;

/// Handle to a particle effect instance
class EZ_PARTICLEPLUGIN_DLL ezParticleEffectHandle
{
  EZ_DECLARE_HANDLE_TYPE(ezParticleEffectHandle, ezParticleEffectId);
};


/// Names of the particle streams that are used by the modules.
///
/// Modules create their streams every time a system is configured. Creating an ezHashedString from a string requires a mutex, so the names are predefined.
struct EZ_PARTICLEPLUGIN_DLL ezParticleStreamNames
{
  static const ezHashedString s_sAxis;
  static const ezHashedString s_sColor;
  static const ezHashedString s_sEffectID;
  static const ezHashedString s_sLastPosition;
  static const ezHashedString s_sLifeTime;
  static const ezHashedString s_sOnOff;
  static const ezHashedString s_sPosition;
  static const ezHashedString s_sRotationOffset;
  static const ezHashedString s_sRotationSpeed;
  static const ezHashedString s_sSize;
  static const ezHashedString s_sTrailData;
  static const ezHashedString s_sVariation;
  static const ezHashedString s_sVelocity;
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
    RandomXAnimatedY,  ///< Random X column, animate Y over lifetime

    Default = None
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleTextureAtlasType);

//////////////////////////////////////////////////////////////////////////

/// Which edge of the source texture is treated as "forward" (i.e. unrotated)
struct EZ_PARTICLEPLUGIN_DLL ezParticleTextureAtlasOrientation
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Up,    ///< Texture's authored "forward" edge is unrotated (current/default behavior)
    Right, ///< Cell content rotated 90° clockwise
    Down,  ///< Cell content rotated 180°
    Left,  ///< Cell content rotated 270° clockwise

    Default = Up
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleTextureAtlasOrientation);

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
