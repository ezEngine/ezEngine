#include <ParticlePluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

// clang-format off

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

ezPlugin g_Plugin(false);

//////////////////////////////////////////////////////////////////////////


EZ_STATICLINK_LIBRARY(ParticlePlugin)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Age);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_ColorGradient);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Gravity);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Raycast);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_SizeCurve);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Behavior_ParticleBehavior_Velocity);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Components_ParticleComponent);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectController);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectDescriptor);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Effect_ParticleEffectInstance);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_Continuous);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Emitter_ParticleEmitter_OnEvent);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Events_ParticleEvent);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_BoxPosition);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_CylinderPosition);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomColor);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomRotationSpeed);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_RandomSize);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_Rise);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_SpherePosition);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Initializer_ParticleInitializer_VelocityCone);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Renderer_ParticleExtractor);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Renderer_ParticleRenderer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Resources_ParticleEffectResource);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Startup);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Streams_DefaultParticleStreams);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Streams_ParticleStream);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_System_ParticleSystemDescriptor);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_System_ParticleSystemInstance);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Billboard_BillboardRenderer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Billboard_ParticleTypeBillboard);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Effect_ParticleTypeEffect);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Fragment_FragmentRenderer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Fragment_ParticleTypeFragment);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Light_ParticleTypeLight);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_ParticleType);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Point_ParticleTypePoint);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Point_PointRenderer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Sprite_ParticleTypeSprite);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Sprite_SpriteRenderer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Trail_ParticleTypeTrail);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Type_Trail_TrailRenderer);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_Util_ParticleUtils);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleEffects);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleSystems);
  EZ_STATICLINK_REFERENCE(ParticlePlugin_WorldModule_ParticleWorldModule);
}
