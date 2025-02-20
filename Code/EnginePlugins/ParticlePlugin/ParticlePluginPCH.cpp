#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

// clang-format off

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezParticleTypeRenderMode, 1)
  EZ_ENUM_CONSTANT(ezParticleTypeRenderMode::Opaque),
  EZ_ENUM_CONSTANT(ezParticleTypeRenderMode::Additive),
  EZ_ENUM_CONSTANT(ezParticleTypeRenderMode::Blended),
  EZ_ENUM_CONSTANT(ezParticleTypeRenderMode::BlendedForeground),
  EZ_ENUM_CONSTANT(ezParticleTypeRenderMode::BlendedBackground),
  EZ_ENUM_CONSTANT(ezParticleTypeRenderMode::Distortion),
  EZ_ENUM_CONSTANT(ezParticleTypeRenderMode::BlendAdd),
EZ_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezParticleLightingMode, 1)
  EZ_ENUM_CONSTANT(ezParticleLightingMode::Fullbright),
  EZ_ENUM_CONSTANT(ezParticleLightingMode::VertexLit),
EZ_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezEffectInvisibleUpdateRate, 1)
  EZ_ENUM_CONSTANT(ezEffectInvisibleUpdateRate::FullUpdate),
  EZ_ENUM_CONSTANT(ezEffectInvisibleUpdateRate::Max20fps),
  EZ_ENUM_CONSTANT(ezEffectInvisibleUpdateRate::Max10fps),
  EZ_ENUM_CONSTANT(ezEffectInvisibleUpdateRate::Max5fps),
  EZ_ENUM_CONSTANT(ezEffectInvisibleUpdateRate::Pause),
  EZ_ENUM_CONSTANT(ezEffectInvisibleUpdateRate::Discard),
EZ_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezParticleTextureAtlasType, 1)
  EZ_ENUM_CONSTANT(ezParticleTextureAtlasType::None),
  EZ_ENUM_CONSTANT(ezParticleTextureAtlasType::RandomVariations),
  EZ_ENUM_CONSTANT(ezParticleTextureAtlasType::FlipbookAnimation),
  EZ_ENUM_CONSTANT(ezParticleTextureAtlasType::RandomYAnimatedX),
EZ_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezParticleColorGradientMode, 1)
  EZ_ENUM_CONSTANT(ezParticleColorGradientMode::Age),
  EZ_ENUM_CONSTANT(ezParticleColorGradientMode::Speed),
EZ_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezParticleOutOfBoundsMode, 1)
  EZ_ENUM_CONSTANT(ezParticleOutOfBoundsMode::Teleport),
  EZ_ENUM_CONSTANT(ezParticleOutOfBoundsMode::Die),
EZ_END_STATIC_REFLECTED_ENUM;

//////////////////////////////////////////////////////////////////////////

// clang-format on

EZ_STATICLINK_LIBRARY(ParticlePlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Bounds);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_ColorGradient);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_FadeOut);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Flies);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Gravity);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_PullAlong);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Raycast);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_SizeCurve);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Velocity);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Components_ParticleComponent);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Components_ParticleFinisherComponent);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectDescriptor);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectInstance);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Burst);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Continuous);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Distance);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_OnEvent);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEventReaction);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEventReaction_Effect);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEventReaction_Prefab);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_Age);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_ApplyVelocity);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_LastPosition);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Finalizer_ParticleFinalizer_Volume);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_BoxPosition);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_CylinderPosition);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomColor);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomRotationSpeed);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomSize);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_SpherePosition);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_VelocityCone);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Module_ParticleModule);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Renderer_ParticleRenderer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Resources_ParticleEffectResource);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Startup);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Streams_DefaultParticleStreams);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Streams_ParticleStream);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_System_ParticleSystemDescriptor);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Effect_ParticleTypeEffect);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Light_ParticleTypeLight);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Mesh_ParticleTypeMesh);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_ParticleType);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Point_ParticleTypePoint);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Point_PointRenderer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Quad_ParticleTypeQuad);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Quad_QuadParticleRenderer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Trail_ParticleTypeTrail);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Trail_TrailRenderer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleWorldModule);
}
