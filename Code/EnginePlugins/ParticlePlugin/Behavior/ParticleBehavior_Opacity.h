#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Behavior that modifies particle alpha over their lifetime using a curve
///
/// The curve is sampled based on the particle's normalized lifetime (0-1).
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Opacity final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Opacity, ezParticleBehavior);

public:

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

  // ************************************* PROPERTIES ***********************************

  ezEnum<ezCurveSource> m_CurveSource;
  ezSingleCurveData m_Curve;
  ezCurve1DResourceHandle m_hSharedCurve;
  mutable ezCurve1D m_RuntimeCurve;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamLifeTime = nullptr;
    ezProcessingStream* m_pStreamColor = nullptr;
    ezUInt8 m_uiFirstToUpdate = 0;
    ezUInt8 m_uiCurrentUpdateInterval = 2;
  };
};
