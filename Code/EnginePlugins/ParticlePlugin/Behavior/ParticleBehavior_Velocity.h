#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

/// How velocity behavior changes particle speed
struct EZ_PARTICLEPLUGIN_DLL ezVelocityChangeMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    CustomCurve, ///< Use embedded curve data
    SharedCurve, ///< Reference shared curve resource
    Friction,    ///< Apply friction to reduce speed

    Default = Friction
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PARTICLEPLUGIN_DLL, ezVelocityChangeMode);

/// Behavior that applies friction and rise/fall forces to particles
///
/// Friction reduces particle velocity over time.
/// Rise speed makes particles move along the inverse gravity direction.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Velocity final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Velocity, ezParticleBehavior);

public:
  ezParticleBehavior_Velocity();
  ~ezParticleBehavior_Velocity();


  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  float m_fFriction = 0;
  ezEnum<ezVelocityChangeMode> m_ChangeSpeedWith;
  ezSingleCurveData m_SpeedCurve;
  ezCurve1DResourceHandle m_hSpeedSharedCurve;
  float m_fSpeedCurveOffset = 0.0f;
  float m_fSpeedCurveScale = 1.0f;
  mutable ezCurve1D m_RuntimeSpeedCurve;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamVelocity;
    ezProcessingStream* m_pStreamLifeTime;
  };
};
