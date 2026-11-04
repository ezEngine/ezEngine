#pragma once

#include <Foundation/Strings/String.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

/// How particles react when hitting a physics surface
struct EZ_PARTICLEPLUGIN_DLL ezParticleRaycastHitReaction
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Bounce, ///< Particle bounces off the surface
    Die,    ///< Particle is killed on impact
    Stop,   ///< Particle stops moving

    Default = Bounce
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezParticleRaycastHitReaction);

/// Behavior that performs physics raycasts to detect collisions
///
/// Raycasts from the particle's last position to its current position.
/// On collision, particles can bounce, die, or stop.
/// Optionally triggers an event on collision.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Raycast final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Raycast, ezParticleBehavior);

public:
  ezParticleBehavior_Raycast();
  ~ezParticleBehavior_Raycast();


  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void QueryOptionalStreams(const ezParticleModuleContext& ctx) const override;
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;
  virtual void RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule) const override;

  ezEnum<ezParticleRaycastHitReaction> m_Reaction; ///< How particles react to collisions
  ezUInt8 m_uiCollisionLayer = 0;                  ///< Physics collision layer to raycast against
  ezHashedString m_sOnCollideEvent;                ///< Optional event name to raise on collision
  float m_fBounceFactor = 0.5f;                    ///< Velocity multiplier when bouncing (energy loss)
  float m_fSlideFactor = 0.5f;                     ///< How much particles slide along the surface
  float m_fSizeFactor = 0.1f;                      ///< Multiplier for particle size used in raycast length

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezPhysicsWorldModuleInterface* m_pPhysicsModule;
    ezProcessingStream* m_pStreamPosition;
    ezProcessingStream* m_pStreamLastPosition;
    ezProcessingStream* m_pStreamVelocity;
    const ezProcessingStream* m_pStreamSize;
  };
};
